/*
* Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     hujianbo <hujianbo@uniontech.com>
*
* Maintainer: hujianbo <hujianbo@uniontech.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "thumbnailsbar.h"
#include "camview.h"
#include <sys/time.h>
#include <QCollator>
#include <DLabel>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMenu>
#include <QMenuBar>
#include <QAction>
#include <DDesktopServices>
#include <QDir>
#include <QProcess>
#include <QDateTime>
#include <QApplication>
#include <QMimeData>
#include <QClipboard>
#include <QKeyEvent>
#include <QShortcut>
//QMap<QString, QPixmap> m_imagemap;
static QMap<int, ImageItem *> m_indexImage;
static int _indexNow = 0;
static QSet<int> m_index;
static bool m_bMulti = false; //是否多选
bool compareByString(const DBImgInfo &str1, const DBImgInfo &str2)
{
    static QCollator sortCollator;

    sortCollator.setNumericMode(true);

    return sortCollator.compare(str1.fileName, str2.fileName) < 0;
}
ImageItem::ImageItem(int index, QString path, QWidget *parent)
{
    Q_UNUSED(parent);
    _index = index;
    _path = path;
    //    if (m_imagemap.contains(path)) {
    //        m_imagemap.value(path);
    //    }
    //    QPixmap pixmap = QPixmap::fromImage(QImage(path));
    //    m_imagemap.insert(path, pixmap.scaledToHeight(THUMBNAIL_HEIGHT,  Qt::FastTransformation));
    //    _image = new DLabel(this);
    //    _image->setPixmap(_pixmap);
    //    connect(dApp, &Application::sigFinishLoad, this, [ = ](QString mapPath) {
    //        if (mapPath == _path || mapPath == "") {
    //            bFirstUpdate = false;
    //            if (m_imagemap.contains(_path)) {
    //                _pixmap = m_imagemap.value(_path);
    //                update();
    //            }
    //        }
    //    });
}

void ImageItem::mouseDoubleClickEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev);
    QFileInfo fileInfo(_path);
    if (fileInfo.suffix() == "jpg") {
        QString program = "deepin-image-viewer"; //用看图打开
        QStringList arguments;
        arguments << _path;
        QProcess *myProcess = new QProcess(this);
        myProcess->startDetached(program, arguments);
    } else {
        QString program = "deepin-movie"; //用影院打开
        QStringList arguments;
        arguments << _path;
        QProcess *myProcess = new QProcess(this);
        myProcess->startDetached(program, arguments);
    }
}

void ImageItem::mouseReleaseEvent(QMouseEvent *ev) //改到缩略图里边重载，然后set到indexnow，现在的方法只是重绘了这一个item
{
    if (ev->button() == Qt::LeftButton) {
        if (_index != _indexNow) {
            //ImageItem *tItem = m_indexImage.value(_indexNow);
            //tItem->update();
            _indexNow = _index;
            update();
        }
    }
    //    if (m_bMulti) {
    //        if (m_index.contains(_index)) {
    //            m_index.remove(_index);
    //        }
    //        else {
    //            m_index.insert(_index);
    //        }
    //    }
}

void ImageItem::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::RightButton) {
        if (_index != _indexNow) {
            //ImageItem *tItem = m_indexImage.value(_indexNow);
            //tItem->update();
            _indexNow = _index;
            update();
        }
    }
    if (m_bMulti && ev->button() == Qt::LeftButton) { //左键选择，右键腾出来用于选择菜单
        if (m_index.contains(_index)) {
            m_index.remove(_index);
        } else {
            m_index.insert(_index);
        }
    }
}
void ImageItem::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    DGuiApplicationHelper::ColorType themeType = DGuiApplicationHelper::instance()->themeType();
    qDebug() << "paint" << _index;
    QPainter painter(this);

    painter.setRenderHints(QPainter::HighQualityAntialiasing | QPainter::SmoothPixmapTransform | QPainter::Antialiasing);
    QRect backgroundRect = rect();
    QRect pixmapRect;
    QFileInfo fileinfo(_path);
    QString str = fileinfo.suffix();
    if (m_index.contains(_index) || (m_index.isEmpty() && _index == _indexNow)) {
        QPainterPath backgroundBp;
        QRect reduceRect = QRect(backgroundRect.x() + 1, backgroundRect.y() + 1,
                                 backgroundRect.width() - 2, backgroundRect.height() - 2);
        backgroundBp.addRoundedRect(reduceRect, 8, 8);
        painter.setClipPath(backgroundBp);
        painter.fillRect(
            reduceRect,
            QBrush(DGuiApplicationHelper::instance()->applicationPalette().highlight().color()));

        if (_pixmap.width() > _pixmap.height()) {
            _pixmap = _pixmap.copy((_pixmap.width() - _pixmap.height()) / 2, 0, _pixmap.height(),
                                   _pixmap.height());
        } else if (_pixmap.width() < _pixmap.height()) {
            _pixmap = _pixmap.copy(0, (_pixmap.height() - _pixmap.width()) / 2, _pixmap.width(),
                                   _pixmap.width());
        }

        pixmapRect.setX(backgroundRect.x() + 5);
        pixmapRect.setY(backgroundRect.y() + 5);
        pixmapRect.setWidth(backgroundRect.width() - 10);
        pixmapRect.setHeight(backgroundRect.height() - 10);
        //修复透明图片被选中后透明地方变成绿色
        QPainterPath bg0;
        bg0.addRoundedRect(pixmapRect, 4, 4);
        painter.setClipPath(bg0);
        if (themeType == DGuiApplicationHelper::LightType)
            painter.fillRect(pixmapRect, QBrush(Qt::white));
        else if (themeType == DGuiApplicationHelper::DarkType)
            painter.fillRect(pixmapRect, QBrush(Qt::black));
        if (!_pixmap.isNull()) {
            //            painter.fillRect(pixmapRect,
            //            QBrush(DGuiApplicationHelper::instance()->applicationPalette().frameBorder().color()));
        }

        //        if (themeType == DGuiApplicationHelper::DarkType) {
        //            if(bFirstUpdate)
        //                m_pixmapstring = LOCMAP_SELECTED_DARK;
        //            else
        //                m_pixmapstring = LOCMAP_SELECTED_DAMAGED_DARK;
        //        } else {
        //            if(bFirstUpdate)
        //                m_pixmapstring = LOCMAP_SELECTED_LIGHT;
        //            else
        //                m_pixmapstring = LOCMAP_SELECTED_DAMAGED_LIGHT;
        //        }

        //        QPixmap pixmap = utils::base::renderSVG(m_pixmapstring, QSize(60, 60));
        QPainterPath bg;
        bg.addRoundedRect(pixmapRect, 4, 4);
        if (_pixmap.isNull()) {
            painter.setClipPath(bg);
            //            painter.drawPixmap(pixmapRect, m_pixmapstring);
            QIcon icon(m_pixmapstring);
            icon.paint(&painter, pixmapRect);
        }
        this->setFixedSize(48, 58);
    } else {
        pixmapRect.setX(backgroundRect.x() + 1);
        pixmapRect.setY(backgroundRect.y() + 0);
        pixmapRect.setWidth(backgroundRect.width() - 2);
        pixmapRect.setHeight(backgroundRect.height() - 0);

        QPainterPath bg0;
        bg0.addRoundedRect(pixmapRect, 4, 4);
        painter.setClipPath(bg0);

        if (!_pixmap.isNull()) {
            //            painter.fillRect(pixmapRect,
            //            QBrush(DGuiApplicationHelper::instance()->applicationPalette().frameBorder().color()));
        }

        //        if (themeType == DGuiApplicationHelper::DarkType) {
        //            if(bFirstUpdate)
        //                m_pixmapstring = LOCMAP_NOT_SELECTED_DARK;
        //            else
        //                m_pixmapstring = LOCMAP_NOT_SELECTED_DAMAGED_DARK;
        //        } else {
        //            if(bFirstUpdate)
        //                m_pixmapstring = LOCMAP_NOT_SELECTED_LIGHT;
        //            else
        //                m_pixmapstring = LOCMAP_NOT_SELECTED_DAMAGED_LIGHT;
        //        }

        //        QPixmap pixmap = utils::base::renderSVG(m_pixmapstring, QSize(30, 40));
        QPainterPath bg;
        bg.addRoundedRect(pixmapRect, 4, 4);
        if (_pixmap.isNull()) {
            painter.setClipPath(bg);
            //            painter.drawPixmap(pixmapRect, m_pixmapstring);

            QIcon icon(m_pixmapstring);
            icon.paint(&painter, pixmapRect);
        }
        this->setFixedSize(30, 40);
    }
    //    QPixmap blankPix = _pixmap;
    //    blankPix.fill(Qt::white);

    //    QRect whiteRect;
    //    whiteRect.setX(pixmapRect.x() + 1);
    //    whiteRect.setY(pixmapRect.y() + 1);
    //    whiteRect.setWidth(pixmapRect.width() - 2);
    //    whiteRect.setHeight(pixmapRect.height() - 2);

    QPainterPath bg1;
    bg1.addRoundedRect(pixmapRect, 4, 4);
    painter.setClipPath(bg1);

    //    painter.drawPixmap(pixmapRect, blankPix);
    painter.drawPixmap(pixmapRect, _pixmap);

    painter.save();
    painter.setPen(
        QPen(DGuiApplicationHelper::instance()->applicationPalette().frameBorder().color(), 1));
    painter.drawRoundedRect(pixmapRect, 4, 4);
    painter.restore();
}

ThumbnailsBar::ThumbnailsBar(DWidget *parent) : DFloatingWidget(parent)
{
    this->grabKeyboard(); //获取键盘事件的关键处理
    QShortcut *shortcut = new QShortcut(QKeySequence(tr("ctrl+c")), this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(onShortcutCopy()));
    QShortcut *shortcutDel = new QShortcut(QKeySequence(Qt::Key_Delete), this);
    connect(shortcutDel, SIGNAL(activated()), this, SLOT(onShortcutDel()));

    m_nStatus = STATNULL;
    m_nActTpye = ActTakePic;
    m_nItemCount = 0;
    m_nMaxItem = 0;
    m_hBOx = new QHBoxLayout();
    m_hBOx->setSpacing(2);
    m_mainLayout = new QHBoxLayout();
    m_mainLayout->setContentsMargins(5, 0, 5, 0);

    setBlurBackgroundEnabled(true); //设置磨砂效果

    m_mainLayout->setObjectName(QStringLiteral("horizontalLayout_4"));

    m_mainLayout->setObjectName(QStringLiteral("horizontalLayout_5"));

    m_mainLayout->addLayout(m_hBOx, Qt::AlignLeft);
    //    QSpacerItem *horizontalSpacer;
    //    horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    //    m_mainLayout->addItem(horizontalSpacer);

    m_lastButton = new DPushButton(this);

    m_lastButton->setFixedWidth(64);
    m_lastButton->setFixedHeight(50);
    QIcon iconPic(":/images/icons/button/photograph.svg");
    m_lastButton->setIcon(iconPic);
    m_lastButton->setIconSize(QSize(24, 20));

    DPalette pa = m_lastButton->palette();
    QColor clo("#0081FF");
    pa.setColor(DPalette::Dark, clo);
    pa.setColor(DPalette::Light, clo);
    m_lastButton->setPalette(pa);

    m_lastButton->setToolTip(tr("Take photo"));
    m_lastButton->setToolTipDuration(500);//0.5s消失

    connect(m_lastButton, SIGNAL(clicked()), this, SLOT(onBtnClick()));

    m_mainLayout->addWidget(m_lastButton, Qt::AlignRight);
    this->setLayout(m_mainLayout);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
}

//void ThumbnailsBar::load()
//{
//    QString path;
//    for (int i = 0; i < m_infos.size(); i ++) {
//        path = m_infos[i].filePath;
//        loadInterface(path);
//    }
//}

//void ThumbnailsBar::resizeEvent(QResizeEvent *size)
//{
//    int nWidth = this->width();

////    printf("resize w %d, h %d\n", size.w);

//}

//void ThumbnailsBar::loadInterface(QString path)
//{
//    QImage tImg;
//    tImg = QImage(path);
//    QPixmap pixmap = QPixmap::fromImage(tImg);

//    //m_writelock.lockForWrite();
//    m_imagemap.insert(path, pixmap.scaledToHeight(IMAGE_HEIGHT_DEFAULT,  Qt::FastTransformation));
//    //m_writelock.unlock();
//}

//待完善内容：1、视频缩略图显示时间;2、文件排序。
void ThumbnailsBar::onFoldersChanged(const QString &strDirectory)
{
    Q_UNUSED(strDirectory);
    m_nItemCount = 0;
    qDebug() << m_nMaxItem;
    int nLetAddCount = (m_nMaxItem - 64 - 20) / (THUMBNAIL_WIDTH + 2) - 1;

    QLayoutItem *child;
    while ((child = m_hBOx->takeAt(0)) != nullptr) {
        //setParent为NULL，防止删除之后界面不消失
        if (child->widget()) {
            child->widget()->setParent(nullptr);
        }

        delete child;
    }
    //    m_imagemap.clear();
    m_indexImage.clear();
    //获取所选文件类型过滤器
    QStringList filters;
    filters << QString("*.jpg") << QString("*.mp4");
    int tIndex = 0;
    QString strFolder;
    for (int i = m_strlstFolders.size(); i >= 1; i--) {
        strFolder = m_strlstFolders[i - 1];
        QDir dir(strFolder);
        //按时间逆序排序
        dir.setNameFilters(filters);
        dir.setSorting(QDir::Time /*| QDir::Reversed*/);
        if (dir.exists()) {
            //使用dir就不要使用迭代器了，会让dir的设置失效
            //QDirIterator dir_iterator(dir);
            QFileInfoList list = dir.entryInfoList();
            for (int i = 0; i < list.size(); ++i) {
                if (nLetAddCount <= m_nItemCount) {
                    break;
                }
                QString strFile = list.at(i).filePath();
                QFileInfo fileInfo = list.at(i);
                //DLabel *pLabel = new DLabel(this);
                ImageItem *pLabel = new ImageItem(tIndex, fileInfo.filePath());
                m_indexImage.insert(tIndex, pLabel);
                tIndex++;
                pLabel->setScaledContents(true);
                pLabel->setFixedSize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT);

                QPixmap *pix = nullptr;
                QImage *tmpimg = nullptr;
                if (fileInfo.suffix() == "mkv" || fileInfo.suffix() == "mp4") {
                    tmpimg = new QImage(":/images/123.jpg");
                    //                    tmpimg->scaled(pLabel->width()-50,pLabel->height()-50);
                    pix = new QPixmap(QPixmap::fromImage(*tmpimg));

                } else if (fileInfo.suffix() == "jpg") {
                    pix = new QPixmap(/*dir_iterator.next()*/ strFile);
                } else {
                    continue; //其他格式不管
                }
                pLabel->updatePic(*pix);
                //pLabel->setPixmap(*pix);

                QMenu *menu = new QMenu();
                //                QAction *actOpen = new QAction(this);//改为双击打开
                //                actOpen->setText("打开");
                QAction *actCopy = new QAction(this);
                actCopy->setText("Copy");

                //actCopy->setShortcut(/*QKeySequence("Ctrl+C")*/QKeySequence(Qt::CTRL | Qt::Key_C));
                //addAction(actCopy);

                QAction *actDel = new QAction(this);
                actDel->setText("Delete");
                QAction *actOpenFolder = new QAction(this);
                actOpenFolder->setText("Open folder");
                //                menu->addAction(actOpen);
                menu->addAction(actCopy);
                menu->addAction(actDel);
                menu->addAction(actOpenFolder);

                pLabel->setContextMenuPolicy(Qt::CustomContextMenu);
                //connect(pLabel, SIGNAL(customContextMenuRequested(QPoint)),this, SLOT(showListWidgetMenuSlot(QPoint)));

                connect(pLabel, &DLabel::customContextMenuRequested, this, [ = ](QPoint pos) {
                    Q_UNUSED(pos);
                    menu->exec(QCursor::pos());
                });
                //                connect(actOpen, &QAction::triggered, this, [ = ] {
                //                    //                    QString  cmd = QString("xdg-open ") + strFile; //在linux下，可以通过system来xdg-open命令调用默认程序打开文件；
                //                    //                    system(cmd.toStdString().c_str());

                //                    if (fileInfo.suffix() == "jpg")
                //                    {
                //                        QString program = "deepin-image-viewer"; //用看图打开
                //                        QStringList arguments;
                //                        arguments << strFile;
                //                        QProcess *myProcess = new QProcess(this);
                //                        myProcess->startDetached(program, arguments);
                //                    } else
                //                    {
                //                        QString program = "deepin-movie"; //用影院打开
                //                        QStringList arguments;
                //                        arguments << strFile;
                //                        QProcess *myProcess = new QProcess(this);
                //                        myProcess->startDetached(program, arguments);
                //                    }
                //                });
                connect(actCopy, &QAction::triggered, this, [ = ] {
                    //                    QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
                    //                                                                    strFile,
                    //                                                                    tr("Images (*.jpg)"));
                    QStringList paths;
                    if (m_index.isEmpty()) {
                        paths = QStringList(strFile);
                        qDebug() << "sigle way";
                    } else {
                        QSet<int>::iterator it;
                        for (it = m_index.begin(); it != m_index.end(); ++it) {
                            paths << m_indexImage.value(*it)->getPath();
                            qDebug() << m_indexImage.value(*it)->getPath();
                        }
                    }

                    QClipboard *cb = qApp->clipboard();
                    QMimeData *newMimeData = new QMimeData();
                    QByteArray gnomeFormat = QByteArray("copy\n");
                    QString text;
                    QList<QUrl> dataUrls;
                    for (QString path : paths) //待添加复制和删除功能以及快捷键效果
                    {
                        if (!path.isEmpty())
                            text += path + '\n';
                        dataUrls << QUrl::fromLocalFile(path);

                        gnomeFormat.append(QUrl::fromLocalFile(path).toEncoded()).append("\n");
                    }

                    newMimeData->setText(text.endsWith('\n') ? text.left(text.length() - 1) : text);
                    newMimeData->setUrls(dataUrls);

                    gnomeFormat.remove(gnomeFormat.length() - 1, 1);
                    //本系统(UOS)特有
                    newMimeData->setData("x-special/gnome-copied-files", gnomeFormat);

                    //                    QImage img(paths.first());//img特有，视频不需要
                    //                    Q_ASSERT(!img.isNull());
                    //                    newMimeData->setImageData(img);

                    cb->setMimeData(newMimeData, QClipboard::Clipboard);
                });
                connect(actOpenFolder, &QAction::triggered, this, [ = ] {
                    //DDesktopServices::trash(strFile);//这个函数是移入回收站

                    QString strtmp = strFolder;
                    if (strtmp.size() && strtmp[0] == '~')
                    {
                        //奇怪，这里不能直接使用strFolder调replace函数
                        strtmp.replace(0, 1, QDir::homePath());
                    }
                    Dtk::Widget::DDesktopServices::showFolder(strtmp);
                });
                connect(actDel, &QAction::triggered, this, [=] {
                    if (m_index.isEmpty()) {
                        //                        QFile filetmp(strFile);
                        //                        filetmp.remove();
                        DDesktopServices::trash(strFile);
                    } else {
                        QSet<int>::iterator it;
                        for (it = m_index.begin(); it != m_index.end(); ++it) {
                            //                            QFile filetmp(m_indexImage.value(*it)->getPath());
                            //                            filetmp.remove();
                            DDesktopServices::trash(m_indexImage.value(*it)->getPath());
                        }
                    }
                });
                //tmpimg->scaled(pLabel->size());

                pix->scaled(pLabel->size(), Qt::KeepAspectRatio);
                m_hBOx->addWidget(pLabel);

                m_nItemCount++;
            }
        }
    }
    emit fitToolBar();
}

void ThumbnailsBar::onBtnClick() //没有相机录像崩溃，待处理
{
    if (m_nActTpye == ActTakePic) {
        if (m_nStatus == STATPicIng) {
            m_nStatus = STATNULL;
            emit enableTitleBar(3);
            emit takePic();
        } else {
            m_nStatus = STATPicIng;
            //1、标题栏视频按钮置灰不可选
            emit enableTitleBar(1);
            emit takePic();
        }

    } else if (m_nActTpye == ActTakeVideo) {
        if (m_nStatus == STATVdIng) {
            m_nStatus = STATNULL;
            emit enableTitleBar(4);
            emit enableSettings(true);
            emit takeVd();
        } else {
            m_nStatus = STATVdIng;
            //1、标题栏拍照按钮置灰不可选
            emit enableTitleBar(2);
            //2、禁用设置功能
            emit enableSettings(false);
            //3、录制
            emit takeVd();

            this->hide();

            m_nItemCount = 0;
            emit fitToolBar();
        }

    } else {
        return;
    }
}

void ThumbnailsBar::onShortcutCopy()
{
    QStringList paths;
    if (m_index.isEmpty()) {
        paths = QStringList(m_indexImage.value(_indexNow)->getPath());
        qDebug() << "sigle way";
    } else {
        QSet<int>::iterator it;
        for (it = m_index.begin(); it != m_index.end(); ++it) {
            paths << m_indexImage.value(*it)->getPath();
            qDebug() << m_indexImage.value(*it)->getPath();
        }
    }

    QClipboard *cb = qApp->clipboard();
    QMimeData *newMimeData = new QMimeData();
    QByteArray gnomeFormat = QByteArray("copy\n");
    QString text;
    QList<QUrl> dataUrls;
    for (QString path : paths) //待添加复制和删除功能以及快捷键效果
    {
        if (!path.isEmpty())
            text += path + '\n';
        dataUrls << QUrl::fromLocalFile(path);

        gnomeFormat.append(QUrl::fromLocalFile(path).toEncoded()).append("\n");
    }

    newMimeData->setText(text.endsWith('\n') ? text.left(text.length() - 1) : text);
    newMimeData->setUrls(dataUrls);

    gnomeFormat.remove(gnomeFormat.length() - 1, 1);
    //本系统(UOS)特有
    newMimeData->setData("x-special/gnome-copied-files", gnomeFormat);
    cb->setMimeData(newMimeData, QClipboard::Clipboard);
}

void ThumbnailsBar::onShortcutDel()
{
    if (m_index.isEmpty()) {
        DDesktopServices::trash(m_indexImage.value(_indexNow)->getPath());
    } else {
        QSet<int>::iterator it;
        for (it = m_index.begin(); it != m_index.end(); ++it) {
            DDesktopServices::trash(m_indexImage.value(*it)->getPath());
        }
    }
}

void ThumbnailsBar::ChangeActType(int nType)
{
    if (m_nActTpye == nType) {
        return;
    }
    m_nActTpye = nType;
    if (nType == ActTakePic) {
        QIcon iconPic(":/images/icons/button/photograph.svg");
        m_lastButton->setIcon(iconPic);
        m_lastButton->setIconSize(QSize(24, 20));

        DPalette pa = m_lastButton->palette();
        QColor clo("#0081FF");
        pa.setColor(DPalette::Dark, clo);
        pa.setColor(DPalette::Light, clo);
        m_lastButton->setPalette(pa);

        m_lastButton->setToolTip(tr("Take photo"));
    } else if (nType == ActTakeVideo) {
        QIcon iconPic(":/images/icons/button/transcribe.svg");
        m_lastButton->setIcon(iconPic);
        m_lastButton->setIconSize(QSize(26, 16));

        DPalette pa = m_lastButton->palette();
        QColor clo("#FF0000");
        pa.setColor(DPalette::Dark, clo);
        pa.setColor(DPalette::Light, clo);
        m_lastButton->setPalette(pa);

        m_lastButton->setToolTip(tr("Record video"));
    } else {
        return;
    }

}

void ThumbnailsBar::addPath(QString strPath)
{
    if (!m_strlstFolders.contains(strPath)) {
        m_strlstFolders.push_back(strPath);
    }

    onFoldersChanged("");
}

void ThumbnailsBar::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Shift) {
        m_bMulti = true; //完成ctrl+c或者删除等操作后恢复false
        m_index.insert(_indexNow);
    }
}

void ThumbnailsBar::keyReleaseEvent(QKeyEvent *e)
{
    //    if (e->key() == Qt::Key_Shift) {
    //        m_bMulti = false;
    //        m_index.clear();
    //        //不填0就是默认当前选项
    //        //m_index.insert(0);//始终选择第一个，选择当前的话，有可能正好是取消当前选项，此时需要定义选中哪个选项
    //    }
}
