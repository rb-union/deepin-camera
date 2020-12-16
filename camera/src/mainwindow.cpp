/*
* Copyright (C) 2020 ~ 2021 Uniontech Software Technology Co.,Ltd.
*
* Author:     shicetu <shicetu@uniontech.com>
*             hujianbo <hujianbo@uniontech.com>
* Maintainer: shicetu <shicetu@uniontech.com>
*             hujianbo <hujianbo@uniontech.com>
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

#include "mainwindow.h"
#include "capplication.h"
#include "v4l2_core.h"
#include "datamanager.h"
#include "shortcut.h"

#include <DLabel>
#include <DApplication>
#include <DSettingsDialog>
#include <DSettingsOption>
#include <DSettings>
#include <DLineEdit>
#include <DFileDialog>
#include <DDialog>
#include <DDesktopServices>
#include <DMessageBox>
#include <DComboBox>

#include <QListWidgetItem>
#include <QTextLayout>
#include <QStyleFactory>
#include <QFileDialog>
#include <QStandardPaths>
#include <QFormLayout>
#include <DWindowMinButton>
#include <DWindowCloseButton>
#include <DWindowMaxButton>
#include <DWindowOptionButton>

#include <qsettingbackend.h>
#include <dsettingswidgetfactory.h>
#include <QShortcut>
#include <QLibraryInfo>
extern "C" {
#include "encoder.h"
#include "load_libs.h"
}

using namespace dc;

const int CMainWindow::minWindowWidth = 800;
const int CMainWindow::minWindowHeight = 583;

QString CMainWindow::lastVdFileName = {""};
QString CMainWindow::lastPicFileName = {""};

static void workaround_updateStyle(QWidget *parent, const QString &theme)
{
    parent->setStyle(QStyleFactory::create(theme));
    for (auto obj : parent->children()) {
        QWidget *tmp_widget = qobject_cast<QWidget *>(obj);
        if (tmp_widget)
            workaround_updateStyle(tmp_widget, theme);
    }
}

static QString ElideText(const QString &text, const QSize &size,
                         QTextOption::WrapMode wordWrap, const QFont &font,
                         Qt::TextElideMode mode, int lineHeight, int lastLineWidth)
{
    int tmpheight = 0;
    QTextLayout textLayout(text);
    QString textlinestr = nullptr;
    QFontMetrics fontMetrics(font);
    textLayout.setFont(font);
    const_cast<QTextOption *>(&textLayout.textOption())->setWrapMode(wordWrap);
    textLayout.beginLayout();
    QTextLine tmptextline = textLayout.createLine();

    while (tmptextline.isValid()) {
        tmpheight += lineHeight;

        if (tmpheight + lineHeight >= size.height()) {
            textlinestr += fontMetrics.elidedText(text.mid(tmptextline.textStart() + tmptextline.textLength() + 1),
                                                  mode, lastLineWidth);
            break;
        }
        tmptextline.setLineWidth(size.width());
        const QString &tmp_str = text.mid(tmptextline.textStart(), tmptextline.textLength());

        if (tmp_str.indexOf('\n'))
            tmpheight += lineHeight;

        textlinestr += tmp_str;
        tmptextline = textLayout.createLine();

        if (tmptextline.isValid())
            textlinestr.append("\n");
    }

    textLayout.endLayout();

    if (textLayout.lineCount() == 1)
        textlinestr = fontMetrics.elidedText(textlinestr, mode, lastLineWidth);

    return textlinestr;
}

static QWidget *createFormatLabelOptionHandle(QObject *opt)
{
    DTK_CORE_NAMESPACE::DSettingsOption *option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(opt);
    DLabel *lab = new DLabel();
    DWidget *main = new DWidget();
    QHBoxLayout *layout = new QHBoxLayout;
    QWidget *optionWidget = new QWidget;
    QFormLayout *optionLayout = new QFormLayout(optionWidget);

    main->setLayout(layout);
    main->setContentsMargins(0, 0, 0, 0);
    main->setMinimumWidth(240);
    layout->addWidget(lab);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setAlignment(Qt::AlignVCenter);
    lab->setObjectName("OptionFormatLabel");
    lab->setFixedHeight(15);
    lab->setText(option->value().toString());
    QFont font = lab->font();
    font.setPointSize(11);
    lab->setFont(font);
    lab->setAlignment(Qt::AlignVCenter);
    lab->show();
    optionWidget->setObjectName("OptionFrame");
    optionLayout->setContentsMargins(0, 0, 0, 0);
    optionLayout->setSpacing(0);
    optionLayout->addRow(new DLabel(QObject::tr(option->name().toStdString().c_str())), main);
    optionWidget->setContentsMargins(0, 0, 0, 0);
    workaround_updateStyle(optionWidget, "light");

    return optionWidget;
}

static QWidget *createPicSelectableLineEditOptionHandle(QObject *opt)
{
    DTK_CORE_NAMESPACE::DSettingsOption *option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(opt);

    DLineEdit *lineedit = new DLineEdit;
    DWidget *main = new DWidget;
    QHBoxLayout *horboxlayout = new QHBoxLayout;
    DPushButton *icon = new DPushButton(main);
    QWidget *optionWidget = new QWidget;
    QFormLayout *optionLayout = new QFormLayout(optionWidget);
    DDialog *optinvaliddialog = new DDialog(optionWidget);
    static QString nameLast = nullptr;

    main->setLayout(horboxlayout);
    icon->setAutoDefault(false);
    icon->setObjectName("OptionLineEditBtn");
    lineedit->setFixedHeight(30);
    lineedit->setObjectName("OptionSelectableLineEdit");
    lineedit->setText(option->value().toString());
    QFontMetrics tem_fontmetrics = lineedit->fontMetrics();
    QString tmpstrelidetext = ElideText(lineedit->text(), {285, tem_fontmetrics.height()}, QTextOption::WrapAnywhere,
                                        lineedit->font(), Qt::ElideMiddle, tem_fontmetrics.height(), 285);

    option->connect(lineedit, &DLineEdit::focusChanged, [ = ](bool on) {
        if (on)
            lineedit->setText(option->value().toString());
    });

    lineedit->setText(tmpstrelidetext);
    nameLast = tmpstrelidetext;
    icon->setIcon(QIcon(":/images/icons/light/select-normal.svg"));
    icon->setIconSize(QSize(25, 25));
    icon->setFixedHeight(30);
    horboxlayout->addWidget(lineedit);
    horboxlayout->addWidget(icon);
    optionWidget->setObjectName("OptionFrame");
    optionLayout->setContentsMargins(0, 0, 0, 0);
    optionLayout->setSpacing(0);
    main->setMinimumWidth(240);
    optionLayout->addRow(new DLabel(QObject::tr(option->name().toStdString().c_str())), main);
    workaround_updateStyle(optionWidget, "light");
    optinvaliddialog->setObjectName("OptionInvalidDialog");
    optinvaliddialog->setIcon(QIcon(":/images/icons/warning.svg"));
    optinvaliddialog->setMessage(QObject::tr("You don't have permission to operate this folder"));
    optinvaliddialog->setWindowFlags(optinvaliddialog->windowFlags() | Qt::WindowStaysOnTopHint);
    optinvaliddialog->addButton(QObject::tr("Close"), true, DDialog::ButtonRecommend);

    auto validate = [ = ](QString name, bool alert = true) -> bool {
        name = name.trimmed();
        if (name.isEmpty()) return false;

        if (name.size() && name[0] == '~')
        {
            name.replace(0, 1, QDir::homePath());
        }

        QFileInfo fileinfo(name);
        QDir dir(name);
        if (fileinfo.exists())
        {
            if (!fileinfo.isDir()) {
                if (alert) lineedit->showAlertMessage(QObject::tr("Invalid folder"));
                return false;
            }

            if (!fileinfo.isReadable() || !fileinfo.isWritable()) {
                if (alert) lineedit->showAlertMessage(QObject::tr("You don't have permission to operate this folder"));
                return false;
            }
        } else
        {
            if (dir.cdUp()) {
                QFileInfo ch(dir.path());
                if (!ch.isReadable() || !ch.isWritable())
                    return false;
            }
        }

        return true;
    };

    option->connect(icon, &DPushButton::clicked, [ = ]() {
#ifdef UNITTEST
        QString name = "~/Pictures/";
#else
        QString name = DFileDialog::getExistingDirectory(nullptr, QObject::tr("Open folder"),
                                                         CMainWindow::lastOpenedPicPath(),
                                                         DFileDialog::ShowDirsOnly | DFileDialog::DontResolveSymlinks);
#endif

        if (validate(name, false)) {
            option->setValue(name);
            nameLast = name;
        }

        QFileInfo fm(name);

        if ((!fm.isReadable() || !fm.isWritable()) && !name.isEmpty())
            optinvaliddialog->show();

    });

    option->connect(lineedit, &DLineEdit::editingFinished, option, [ = ]() {

        QString name = lineedit->text();
        QDir dir(name);

        QString tempstrname = ElideText(name, {285, tem_fontmetrics.height()}, QTextOption::WrapAnywhere,
                                        lineedit->font(), Qt::ElideMiddle, tem_fontmetrics.height(), 285);
        QString temstrnamelast = ElideText(nameLast, {285, tem_fontmetrics.height()}, QTextOption::WrapAnywhere,
                                           lineedit->font(), Qt::ElideMiddle, tem_fontmetrics.height(), 285);


        if (!validate(lineedit->text(), false)) {
            QFileInfo fn(dir.path());

            if ((!fn.isReadable() || !fn.isWritable()) && !name.isEmpty()) {
                optinvaliddialog->show();
            }
        }

        if (!lineedit->lineEdit()->hasFocus()) {

            if (validate(lineedit->text(), false)) {
                option->setValue(lineedit->text());
                lineedit->setText(tempstrname);
                nameLast = name;
            } else if (tempstrname == tmpstrelidetext) {
                lineedit->setText(tmpstrelidetext);
            } else {
                option->setValue(QVariant(QDir::homePath() + QDir::separator() + "Pictures" + QDir::separator() + QObject::tr("Camera"))); //设置为默认路径
                lineedit->setText(QString(QDir::homePath() + QDir::separator() + "Pictures" + QDir::separator() + QObject::tr("Camera")));
                lineedit->setText(temstrnamelast);
            }
        }
    });

    option->connect(lineedit, &DLineEdit::textEdited, option, [ = ](const QString & newStr) {
        validate(newStr);
    });

    option->connect(option, &DTK_CORE_NAMESPACE::DSettingsOption::valueChanged, lineedit,
    [ = ](const QVariant & value) {
        auto pi = ElideText(value.toString(), {285, tem_fontmetrics.height()}, QTextOption::WrapAnywhere,
                            lineedit->font(), Qt::ElideMiddle, tem_fontmetrics.height(), 285);
        lineedit->setText(pi);
        Settings::get().settings()->setOption("base.general.last_open_pic_path", pi);
        qDebug() << "save pic last path:" << pi << endl;
        lineedit->update();
    });

    return  optionWidget;
}



static QWidget *createVdSelectableLineEditOptionHandle(QObject *opt)
{
    DTK_CORE_NAMESPACE::DSettingsOption *option = qobject_cast<DTK_CORE_NAMESPACE::DSettingsOption *>(opt);

    DLineEdit *lineedit = new DLineEdit;
    DWidget *main = new DWidget;
    QHBoxLayout *horboxlayout = new QHBoxLayout;
    DPushButton *icon = new DPushButton(main);
    QWidget *optionWidget = new QWidget;
    QFormLayout *optionLayout = new QFormLayout(optionWidget);
    DDialog *optinvaliddialog = new DDialog(optionWidget);
    static QString nameLast = nullptr;

    main->setLayout(horboxlayout);
    icon->setAutoDefault(false);
    icon->setObjectName("OptionLineEditBtn");
    lineedit->setFixedHeight(30);
    lineedit->setObjectName("OptionSelectableLineEdit");
    lineedit->setText(option->value().toString());
    QFontMetrics tem_fontmetrics = lineedit->fontMetrics();
    QString tmpstrelidetext = ElideText(lineedit->text(), {285, tem_fontmetrics.height()}, QTextOption::WrapAnywhere,
                                        lineedit->font(), Qt::ElideMiddle, tem_fontmetrics.height(), 285);

    option->connect(lineedit, &DLineEdit::focusChanged, [ = ](bool on) {
        if (on)
            lineedit->setText(option->value().toString());
    });

    lineedit->setText(tmpstrelidetext);
    nameLast = tmpstrelidetext;
    icon->setIcon(QIcon(":/images/icons/light/select-normal.svg"));
    icon->setIconSize(QSize(25, 25));
    icon->setFixedHeight(30);
    horboxlayout->addWidget(lineedit);
    horboxlayout->addWidget(icon);
    optionWidget->setObjectName("OptionFrame");
    optionLayout->setContentsMargins(0, 0, 0, 0);
    optionLayout->setSpacing(0);
    main->setMinimumWidth(240);
    optionLayout->addRow(new DLabel(QObject::tr(option->name().toStdString().c_str())), main);
    workaround_updateStyle(optionWidget, "light");
    optinvaliddialog->setObjectName("OptionInvalidDialog");
    optinvaliddialog->setIcon(QIcon(":/images/icons/warning.svg"));
    optinvaliddialog->setMessage(QObject::tr("You don't have permission to operate this folder"));
    optinvaliddialog->setWindowFlags(optinvaliddialog->windowFlags() | Qt::WindowStaysOnTopHint);
    optinvaliddialog->addButton(QObject::tr("Close"), true, DDialog::ButtonRecommend);

    auto validate = [ = ](QString name, bool alert = true) -> bool {
        name = name.trimmed();
        if (name.isEmpty()) return false;

        if (name.size() && name[0] == '~')
        {
            name.replace(0, 1, QDir::homePath());
        }

        QFileInfo fileinfo(name);
        QDir dir(name);
        if (fileinfo.exists())
        {
            if (!fileinfo.isDir()) {
                if (alert) lineedit->showAlertMessage(QObject::tr("Invalid folder"));
                return false;
            }

            if (!fileinfo.isReadable() || !fileinfo.isWritable()) {
                if (alert) lineedit->showAlertMessage(QObject::tr("You don't have permission to operate this folder"));
                return false;
            }
        } else
        {
            if (dir.cdUp()) {
                QFileInfo ch(dir.path());
                if (!ch.isReadable() || !ch.isWritable())
                    return false;
            }
        }

        return true;
    };

    option->connect(icon, &DPushButton::clicked, [ = ]() {
#ifdef UNITTEST
        QString name = "~/Pictures/";
#else
        QString name = DFileDialog::getExistingDirectory(nullptr, QObject::tr("Open folder"),
                                                         CMainWindow::lastOpenedVideoPath(),
                                                         DFileDialog::ShowDirsOnly | DFileDialog::DontResolveSymlinks);
#endif

        if (validate(name, false)) {
            option->setValue(name);
            nameLast = name;
        }

        QFileInfo fm(name);

        if ((!fm.isReadable() || !fm.isWritable()) && !name.isEmpty())
            optinvaliddialog->show();

    });

    option->connect(lineedit, &DLineEdit::editingFinished, option, [ = ]() {

        QString name = lineedit->text();
        QDir dir(name);

        QString tempstrname = ElideText(name, {285, tem_fontmetrics.height()}, QTextOption::WrapAnywhere,
                                        lineedit->font(), Qt::ElideMiddle, tem_fontmetrics.height(), 285);
        QString temstrnamelast = ElideText(nameLast, {285, tem_fontmetrics.height()}, QTextOption::WrapAnywhere,
                                           lineedit->font(), Qt::ElideMiddle, tem_fontmetrics.height(), 285);


        if (!validate(lineedit->text(), false)) {
            QFileInfo fn(dir.path());

            if ((!fn.isReadable() || !fn.isWritable()) && !name.isEmpty()) {
                optinvaliddialog->show();
            }
        }

        if (!lineedit->lineEdit()->hasFocus()) {

            if (validate(lineedit->text(), false)) {
                option->setValue(lineedit->text());
                lineedit->setText(tempstrname);
                nameLast = name;
            } else if (tempstrname == tmpstrelidetext) {
                lineedit->setText(tmpstrelidetext);
            } else {
                option->setValue(QVariant(QDir::homePath() + QDir::separator() + "Videos" + QDir::separator() + QObject::tr("Camera"))); //设置为默认路径
                lineedit->setText(QString(QDir::homePath() + QDir::separator() + "Videos" + QDir::separator() + QObject::tr("Camera")));
                lineedit->setText(temstrnamelast);
            }
        }
    });

    option->connect(lineedit, &DLineEdit::textEdited, option, [ = ](const QString & newStr) {
        validate(newStr);
    });

    option->connect(option, &DTK_CORE_NAMESPACE::DSettingsOption::valueChanged, lineedit,
    [ = ](const QVariant & value) {
        auto pi = ElideText(value.toString(), {285, tem_fontmetrics.height()}, QTextOption::WrapAnywhere,
                            lineedit->font(), Qt::ElideMiddle, tem_fontmetrics.height(), 285);
        lineedit->setText(pi);
        Settings::get().settings()->setOption("base.general.last_open_vd_path", pi);
        qDebug() << "save pic last path:" << pi << endl;
        lineedit->update();
    });

    return  optionWidget;
}

QString CMainWindow::lastOpenedPicPath()
{
    QString lastPicPath = Settings::get().generalOption("last_open_pic_path").toString();
    qDebug() << lastPicPath << endl;
    QDir lastDir(lastPicPath);

    if (lastPicPath.isEmpty() || !lastDir.exists()) {
        lastPicPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation) + "/" + QObject::tr("Camera");
        QDir newLastDir(lastPicPath);

        if (!newLastDir.exists()) {
            lastPicPath = QDir::currentPath();
        }
    }

    return lastPicPath;
}

QString CMainWindow::lastOpenedVideoPath()
{
    QString lastVdPath = Settings::get().generalOption("last_open_vd_path").toString();
    QDir lastDir(lastVdPath);

    if (lastVdPath.isEmpty() || !lastDir.exists()) {
        lastVdPath = QStandardPaths::writableLocation(QStandardPaths::MoviesLocation) + "/" + QObject::tr("Camera");
        QDir newLastDir(lastVdPath);

        if (!newLastDir.exists()) {
            lastVdPath = QDir::currentPath();
        }
    }

    return lastVdPath;
}

CMainWindow::CMainWindow(DWidget *w): DMainWindow(w)
{
    m_bWayland = false;
    m_nActTpye = ActTakePic;
    setupTitlebar();
}

CMainWindow::~CMainWindow()
{
    if (m_rightbtnmenu) {
        delete m_rightbtnmenu;
        m_rightbtnmenu = nullptr;
    }

    if (m_devnumMonitor) {
        m_devnumMonitor->stop();
        m_devnumMonitor->deleteLater();
        m_devnumMonitor = nullptr;
    }

    if (m_videoPre) {
        m_videoPre->deleteLater();
        m_videoPre = nullptr;
    }

    if (m_thumbnail) {
        m_thumbnail->deleteLater();
        m_thumbnail = nullptr;
    }

    qDebug() << "stop_encoder_thread";
}

void CMainWindow::slotPopupSettingsDialog()
{
    settingDialog();
    m_SetDialog->exec();
    settingDialogDel();
}

QString CMainWindow::libPath(const QString &strlib)
{
    QDir  dir;
    QString path  = QLibraryInfo::location(QLibraryInfo::LibrariesPath);
    dir.setPath(path);
    QStringList list = dir.entryList(QStringList() << (strlib + "*"), QDir::NoDotAndDotDot | QDir::Files); //filter name with strlib

    if (list.contains(strlib))
        return strlib;

    list.sort();
    Q_ASSERT(list.size() > 0);
    return list.last();
}


void CMainWindow::initDynamicLibPath()
{
    LoadLibNames tmp;
    QByteArray avcodec = libPath("libavcodec.so").toLatin1();
    tmp.chAvcodec = avcodec.data();
    QByteArray avformat = libPath("libavformat.so").toLatin1();
    tmp.chAvformat = avformat.data();
    QByteArray avutil = libPath("libavutil.so").toLatin1();
    tmp.chAvutil = avutil.data();
    QByteArray udev = libPath("libudev.so").toLatin1();
    tmp.chUdev = udev.data();
    QByteArray usb = libPath("libusb-1.0.so").toLatin1();
    tmp.chUsb = usb.data();
    QByteArray portaudio = libPath("libportaudio.so").toLatin1();
    tmp.chPortaudio = portaudio.data();
    QByteArray v4l2 = libPath("libv4l2.so").toLatin1();
    tmp.chV4l2 = v4l2.data();
    QByteArray ffmpegthumbnailer = libPath("libffmpegthumbnailer.so").toLatin1();
    tmp.chFfmpegthumbnailer = ffmpegthumbnailer.data();
    QByteArray swscale = libPath("libswscale.so").toLatin1();
    tmp.chSwscale = swscale.data();
    QByteArray swresample = libPath("libswresample.so").toLatin1();
    tmp.chSwresample = swresample.data();
    setLibNames(tmp);
}

void CMainWindow::initBlockShutdown()
{
    if (!m_arg.isEmpty() || m_reply.value().isValid()) {
        qDebug() << "m_reply.value().isValid():" << m_reply.value().isValid();
        return;
    }

    m_pLoginManager = new QDBusInterface("org.freedesktop.login1",
                                         "/org/freedesktop/login1",
                                         "org.freedesktop.login1.Manager",
                                         QDBusConnection::systemBus());

    m_arg << QString("shutdown")             // what
          << qApp->productName()           // who
          << QObject::tr("File not saved")          // why
          << QString("block");                        // mode

    m_reply = m_pLoginManager->callWithArgumentList(QDBus::Block, "Inhibit", m_arg);

    if (m_reply.isValid())
        (void)m_reply.value().fileDescriptor();

    //如果for结束则表示没有发现未保存的tab项，则放开阻塞关机
    if (m_reply.isValid()) {
        QDBusReply<QDBusUnixFileDescriptor> tmp = m_reply;
        m_reply = QDBusReply<QDBusUnixFileDescriptor>();
        qDebug() << "init Nublock shutdown.";
    }
}

void CMainWindow::initBlockSleep()
{
    if (!m_argSleep.isEmpty() || m_replySleep.value().isValid()) {
        qDebug() << "m_reply.value().isValid():" << m_replySleep.value().isValid();
        return;
    }

    m_pLoginMgrSleep = new QDBusInterface("org.freedesktop.login1",
                                          "/org/freedesktop/login1",
                                          "org.freedesktop.login1.Manager",
                                          QDBusConnection::systemBus());

    m_argSleep << QString("sleep")             // what
               << qApp->productName()           // who
               << QObject::tr("File not saved")          // why
               << QString("block");                        // mode

    m_replySleep = m_pLoginMgrSleep->callWithArgumentList(QDBus::Block, "Inhibit", m_argSleep);

    if (m_replySleep.isValid()) {
        (void)m_replySleep.value().fileDescriptor();
    }

    //如果for结束则表示没有发现未保存的tab项，则放开阻塞睡眠
    if (m_replySleep.isValid()) {
        QDBusReply<QDBusUnixFileDescriptor> tmp = m_replySleep;
        m_replySleep = QDBusReply<QDBusUnixFileDescriptor>();
        qDebug() << "init Nublock sleep.";
    }
}

void CMainWindow::initTabOrder()
{
    /*
     *主窗口tab循序切换
    */
    DWindowMinButton *windowMinBtn = titlebar()->findChild<DWindowMinButton *>("DTitlebarDWindowMinButton");
    DWindowOptionButton *windowoptionButton = titlebar()->findChild<DWindowOptionButton *>("DTitlebarDWindowOptionButton");
    DWindowMaxButton *windowMaxBtn = titlebar()->findChild<DWindowMaxButton *>("DTitlebarDWindowMaxButton");
    DWindowCloseButton *windowCloseBtn = titlebar()->findChild<DWindowCloseButton *>("DTitlebarDWindowCloseButton");

    //设置鼠标tab同时切换策略，有一个问题鼠标点击时也会出现一个tab选择框
    setTabOrder(m_pTitlePicBtn, m_pTitleVdBtn);
    setTabOrder(m_pTitleVdBtn, windowoptionButton);
    setTabOrder(windowoptionButton, windowMinBtn);
    setTabOrder(windowMinBtn, windowMaxBtn);
    setTabOrder(windowMaxBtn, windowCloseBtn);
    setTabOrder(windowCloseBtn, m_thumbnail->findChild<DPushButton *>("PicVdBtn"));
    setTabOrder(m_thumbnail->findChild<DPushButton *>("PicVdBtn"), pSelectBtn);
    titlebar()->setFocusPolicy(Qt::ClickFocus);
}

void CMainWindow::initShortcut()
{
    QShortcut *scViewShortcut = new QShortcut(QKeySequence("Ctrl+Shift+/"), this);
    QShortcut *scSpaceShortcut = new QShortcut(QKeySequence("space"), this);

    connect(scViewShortcut, &QShortcut::activated, this, [ = ] {
        qDebug() << "receive Ctrl+Shift+/";
        QRect rect = window()->geometry();
        QPoint pos(rect.x() + rect.width() / 2, rect.y() + rect.height() / 2);
        Shortcut sc;
        QStringList shortcutString;
        QString param1 = "-j=" + sc.toStr();
        QString param2 = "-p=" + QString::number(pos.x()) + "," + QString::number(pos.y());

        shortcutString << "-b" << param1 << param2;
        QProcess::startDetached("deepin-shortcut-viewer", shortcutString);
    });

    connect(scSpaceShortcut, &QShortcut::activated, this, [ = ] {
        DPushButton *tabkevdent =  m_videoPre->findChild<DPushButton *>("TakeVdEndBtn");

        if (tabkevdent->isVisible())
            tabkevdent->click();
        else
            m_thumbnail->findChild<DPushButton *>("PicVdBtn")->click();
    });
}

void CMainWindow::settingDialog()
{
    m_SetDialog = new DSettingsDialog(this);
    bool isVideos = true;
    m_SetDialog->widgetFactory()->registerWidget("selectableEditpic", createPicSelectableLineEditOptionHandle);
    m_SetDialog->widgetFactory()->registerWidget("selectableEditvd", createVdSelectableLineEditOptionHandle);
    m_SetDialog->widgetFactory()->registerWidget("formatLabel", createFormatLabelOptionHandle);
    m_SetDialog->setObjectName("SettingDialog");
    connect(m_SetDialog, SIGNAL(destroyed()), this, SLOT(onSettingsDlgClose()));

    auto resolutionmodeFamily = Settings::get().settings()->option("outsetting.resolutionsetting.resolution");

    if (get_v4l2_device_handler() != nullptr) {
        //格式索引
        int format_index = v4l2core_get_frame_format_index(
                               get_v4l2_device_handler(),
                               v4l2core_get_requested_frame_format(get_v4l2_device_handler()));
        //分辨率索引
        int resolu_index = v4l2core_get_format_resolution_index(
                               get_v4l2_device_handler(),
                               format_index,
                               v4l2core_get_frame_width(get_v4l2_device_handler()),
                               v4l2core_get_frame_height(get_v4l2_device_handler()));
        int defres = 0;
        //获取当前摄像头的格式表
        v4l2_stream_formats_t *list_stream_formats = v4l2core_get_formats_list(get_v4l2_device_handler());
        //初始化分辨率字符串表
        QStringList resolutionDatabase;
        resolutionDatabase.clear();

        //当前分辨率下标
        if (format_index >= 0 && resolu_index >= 0) {

            //format_index = -1 &&resolu_index = -1 表示设备被占用或者不存在
            for (int i = 0 ; i < list_stream_formats[format_index].numb_res; i++) {

                if ((list_stream_formats[format_index].list_stream_cap[i].width > 0
                        && list_stream_formats[format_index].list_stream_cap[i].height > 0) &&
                        (list_stream_formats[format_index].list_stream_cap[i].width < 7680
                         && list_stream_formats[format_index].list_stream_cap[i].height < 4320) &&
                        ((list_stream_formats[format_index].list_stream_cap[i].width % 8) == 0
                         && (list_stream_formats[format_index].list_stream_cap[i].height % 8) ==  0)) {
                    //加入分辨率的字符串
                    QString res_str = QString("%1x%2").arg(list_stream_formats[format_index].list_stream_cap[i].width).arg(list_stream_formats[format_index].list_stream_cap[i].height);

                    //去重
                    if (resolutionDatabase.contains(res_str) == false)
                        resolutionDatabase.append(res_str);
                }
            }

            int tempostion = 0;
            int len = resolutionDatabase.size() - 1;

            for (int i = 0; i < resolutionDatabase.size() - 1; i++) {
                int flag = 1;

                for (int j = 0 ; j < len; j++) {
                    QStringList resolutiontemp1 = resolutionDatabase[j].split("x");
                    QStringList resolutiontemp2 = resolutionDatabase[j + 1].split("x");

                    if ((resolutiontemp1[0].toInt() <= resolutiontemp2[0].toInt())
                            && (resolutiontemp1[1].toInt() < resolutiontemp2[1].toInt())) {
                        QString resolutionstr = resolutionDatabase[j + 1];
                        resolutionDatabase[j + 1] = resolutionDatabase[j];
                        resolutionDatabase[j] = resolutionstr;
                        flag = 0;
                        tempostion = j;
                    }
                }
                len = tempostion;

                if (flag == 1)
                    continue;
            }

            for (int i = 0; i < resolutionDatabase.size(); i++) {
                QStringList resolutiontemp = resolutionDatabase[i].split("x");

                if ((v4l2core_get_frame_width(get_v4l2_device_handler()) == resolutiontemp[0].toInt()) &&
                        (v4l2core_get_frame_height(get_v4l2_device_handler()) == resolutiontemp[1].toInt())) {
                    defres = i; //设置分辨率下拉菜单当前索引
                    break;
                }
            }

            resolutionmodeFamily->setData("items", resolutionDatabase);
            //设置当前分辨率的索引
            resolutionDatabase.append(QString(tr("None")));
            Settings::get().settings()->setOption(QString("outsetting.resolutionsetting.resolution"), defres);
        } else {
            resolutionDatabase.clear();
            resolutionmodeFamily->setData("items", resolutionDatabase);
        }
    } else {
        //初始化分辨率字符串表
        QStringList resolutionDatabase = resolutionmodeFamily->data("items").toStringList();

        if (resolutionDatabase.size() > 0)
            resolutionmodeFamily->data("items").clear();

        resolutionDatabase.clear();
        resolutionDatabase.append(QString(tr("None")));
        Settings::get().settings()->setOption(QString("outsetting.resolutionsetting.resolution"), 0);
        resolutionmodeFamily->setData("items", resolutionDatabase);
    }
    m_SetDialog->setProperty("_d_QSSThemename", "dark");
    m_SetDialog->setProperty("_d_QSSFilename", "DSettingsDialog");
    m_SetDialog->updateSettings(Settings::get().settings());

    QPushButton *resetbtn = m_SetDialog->findChild<QPushButton *>("SettingsContentReset");
    resetbtn->setDefault(false);
    resetbtn->setAutoDefault(false);
}

void CMainWindow::settingDialogDel()
{
    if (m_SetDialog) {
        delete m_SetDialog;
        m_SetDialog = nullptr;
    }
}

void CMainWindow::loadAfterShow()
{
    initDynamicLibPath();
    //该方法导致键盘可用性降低，调试时无法使用、触摸屏无法唤起多次右键菜单，改用备用方案
    //this->grabKeyboard();//与方法：“QGuiApplication::keyboardModifiers() == Qt::ShiftModifier”具有同等效果
    initUI();
    initShortcut();
    gviewencoder_init();
    v4l2core_init();
    m_devnumMonitor = new DevNumMonitor();
    m_pDBus = new QDBusInterface("org.freedesktop.login1", "/org/freedesktop/login1",
                                 "org.freedesktop.login1.Manager", QDBusConnection::systemBus());
    m_devnumMonitor->setParent(this);
    m_devnumMonitor->setObjectName("DevMonitorThread");
    m_devnumMonitor->init();
    m_devnumMonitor->start();
    initTitleBar();
    initConnection();
    QDir dir;
    QString strCache = QString(getenv("HOME")) + QString("/") + QString(".cache/deepin/deepin-camera/");
    dir.mkpath(strCache);
    initThumbnails();
    initThumbnailsConn();
    initTabOrder();

    connect(m_devnumMonitor, SIGNAL(seltBtnStateEnable()), this, SLOT(setSelBtnShow()));//显示切换按钮
    connect(m_devnumMonitor, SIGNAL(seltBtnStateDisable()), this, SLOT(setSelBtnHide()));//多设备信号
    connect(m_devnumMonitor, SIGNAL(existDevice()), m_videoPre, SLOT(onRestartDevices()));//重启设备
    connect(m_pDBus, SIGNAL(PrepareForSleep(bool)), this, SLOT(onSleepWhenTaking(bool)));//接收休眠信号，仅wayland使用

    m_thumbnail->addPath(CMainWindow::lastVdFileName);
    m_thumbnail->addPath(CMainWindow::lastPicFileName);
    m_videoPre->delayInit();
}

void CMainWindow::updateBlockSystem(bool bTrue)
{
    initBlockShutdown();

    if (bTrue)
        m_reply = m_pLoginManager->callWithArgumentList(QDBus::Block, "Inhibit", m_arg);
    else {
        QDBusReply<QDBusUnixFileDescriptor> tmp = m_reply;
        m_reply = QDBusReply<QDBusUnixFileDescriptor>();
        qDebug() << "Nublock shutdown.";
    }

    if (m_bWayland) {
        initBlockSleep();

        if (bTrue)
            m_replySleep = m_pLoginMgrSleep->callWithArgumentList(QDBus::Block, "Inhibit", m_argSleep);
        else {
            QDBusReply<QDBusUnixFileDescriptor> tmp = m_replySleep;
            m_replySleep = QDBusReply<QDBusUnixFileDescriptor>();
            qDebug() << "Nublock sleep.";
        }
    }
}

void CMainWindow::onNoCam()
{
    onEnableTitleBar(3); //恢复按钮状态
    onEnableTitleBar(4); //恢复按钮状态
    onEnableSettings(true);

    if (m_thumbnail) {
        m_thumbnail->m_nStatus = STATNULL;
        m_thumbnail->show();
    }
}

void CMainWindow::onSleepWhenTaking(bool bTrue)
{
    if (m_bWayland && bTrue) {
        qDebug() << "onSleepWhenTaking(bool)";
        m_videoPre->onEndBtnClicked();
        qDebug() << "onSleepWhenTaking(over)";
    }
}

void CMainWindow::initUI()
{
    m_videoPre = new videowidget(this);
    QPalette paletteTime = m_videoPre->palette();
    m_videoPre->setObjectName("VideoPreviewWidget");
    setCentralWidget(m_videoPre);
    paletteTime.setBrush(QPalette::Dark, QColor(/*"#202020"*/0, 0, 0, 51)); //深色
    m_videoPre->setPalette(paletteTime);

    CMainWindow::lastVdFileName = CMainWindow::lastOpenedVideoPath();
    CMainWindow::lastPicFileName = CMainWindow::lastOpenedPicPath();

    if (CMainWindow::lastVdFileName.size() && CMainWindow::lastVdFileName[0] == '~') {
        QString str = QDir::homePath();
        CMainWindow::lastVdFileName.replace(0, 1, str);
    }

    if (CMainWindow::lastPicFileName.size() && CMainWindow::lastPicFileName[0] == '~') {
        QString str = QDir::homePath();
        CMainWindow::lastPicFileName.replace(0, 1, str);
    }

    QDir dir;

    if (QDir(QString(QDir::homePath() + QDir::separator() + "Pictures" + QDir::separator() + QObject::tr("Camera"))).exists() == false)
        dir.mkdir(QDir::homePath() + QDir::separator() + "Pictures" + QDir::separator() + QObject::tr("Camera"));

    if (QDir(QString(QDir::homePath() + QDir::separator() + "Videos" + QDir::separator() + QObject::tr("Camera"))).exists() == false)
        dir.mkdir(QDir::homePath() + QDir::separator() + "Videos" + QDir::separator() + QObject::tr("Camera"));

    bool picturepathexist = false;//判断图片路径是否存在

    if (QDir(CMainWindow::lastVdFileName).exists()) {
        m_fileWatcher.addPath(CMainWindow::lastVdFileName);
    }

    if (QDir(CMainWindow::lastPicFileName).exists()) {
        m_fileWatcher.addPath(CMainWindow::lastPicFileName);
        picturepathexist = true;//图片路径存在
    }

    //缩略图延后加载
    if (picturepathexist)
        m_videoPre->setSaveFolder(CMainWindow::lastPicFileName);
    else
        m_videoPre->setSaveFolder(QDir::homePath() + QDir::separator() + "Pictures" + QDir::separator() + QObject::tr("Camera"));

    int nContinuous = Settings::get().getOption("photosetting.photosnumber.takephotos").toInt();
    int nDelayTime = Settings::get().getOption("photosetting.photosdelay.photodelays").toInt();
    bool soundphoto = Settings::get().getOption("photosetting.audiosetting.soundswitchbtn").toBool();

    switch (nContinuous) {//连拍次数
    case 1:
        nContinuous = 4;
        break;
    case 2:
        nContinuous = 10;
        break;
    default:
        nContinuous = 1;
        break;
    }

    switch (nDelayTime) {//延迟时间
    case 1:
        nDelayTime = 3;
        break;
    case 2:
        nDelayTime = 6;
        break;
    default:
        nDelayTime = 0;
        break;
    }

    if (soundphoto)
        set_takeing_photo_sound(1);
    else
        set_takeing_photo_sound(0);

    m_videoPre->setInterval(nDelayTime);
    m_videoPre->setContinuous(nContinuous);
    resize(CMainWindow::minWindowWidth, CMainWindow::minWindowHeight);
}

void CMainWindow::initTitleBar()
{
    DGuiApplicationHelper::ColorType type = DGuiApplicationHelper::instance()->themeType();
    pDButtonBox = new DButtonBox(this);
    QList<DButtonBoxButton *> listButtonBox;
    m_pTitlePicBtn = new DButtonBoxButton(QString(""), this);
    m_pTitleVdBtn = new DButtonBoxButton(QString(""), this);
    pSelectBtn = new DIconButton(this/*DStyle::SP_IndicatorSearch*/);

    pDButtonBox->setObjectName("myButtonBox");
    pDButtonBox->setFixedWidth(120);
    pDButtonBox->setFixedHeight(36);
    //初始化标题栏拍照按钮
    QIcon iconPic(":/images/icons/light/button/photograph.svg");
    m_pTitlePicBtn->setObjectName("pTitlePicBtn");
    m_pTitlePicBtn->setIcon(iconPic);
    m_pTitlePicBtn->setIconSize(QSize(26, 26));
    DPalette pa = m_pTitlePicBtn->palette();
    QColor clo("#0081FF");
    pa.setColor(DPalette::Dark, clo);
    pa.setColor(DPalette::Light, clo);
    pa.setColor(DPalette::Button, clo);
    m_pTitlePicBtn->setPalette(pa);
    QIcon iconVd;

    //初始化主题判断
    if (type == DGuiApplicationHelper::UnknownType || type == DGuiApplicationHelper::LightType)
        iconVd = QIcon(":/images/icons/light/record video.svg");
    else
        iconVd = QIcon(":/images/icons/dark/button/record video_dark.svg");

    //初始化标题栏录像按钮
    m_pTitleVdBtn->setObjectName("pTitleVdBtn");
    m_pTitleVdBtn->setIcon(iconVd);
    m_pTitleVdBtn->setIconSize(QSize(26, 26));
    listButtonBox.append(m_pTitlePicBtn);
    listButtonBox.append(m_pTitleVdBtn);
    pDButtonBox->setButtonList(listButtonBox, false);
    titlebar()->addWidget(pDButtonBox);

    //初始化切换按钮
    pSelectBtn->setObjectName("SelectBtn");
    pSelectBtn->setFixedSize(QSize(37, 37));
    pSelectBtn->setIconSize(QSize(37, 37));
    pSelectBtn->hide();
    pSelectBtn->setFocusPolicy(Qt::ClickFocus);

    //初始化主题判断
    if (type == DGuiApplicationHelper::UnknownType || type == DGuiApplicationHelper::LightType)
        pSelectBtn->setIcon(QIcon(":/images/icons/light/button/Switch camera.svg"));
    else
        pSelectBtn->setIcon(QIcon(":/images/icons/dark/button/Switch camera_dark.svg"));

    titlebar()->setIcon(QIcon::fromTheme("deepin-camera"));
    titlebar()->addWidget(pSelectBtn, Qt::AlignLeft);
}

void CMainWindow::initConnection()
{
    connect(dApp, &CApplication::popupConfirmDialog, this, [ = ] {
        if (m_videoPre->getCapStatus())
        {
            CloseDialog closeDlg(this, tr("Video recording is in progress. Close the window?"));
#ifdef UNITTEST
            closeDlg.show();
            int ret = 1;
#else
            int ret = closeDlg.exec();
#endif
            if (ret == 1) {
                m_videoPre->onEndBtnClicked();
#ifdef UNITTEST
                closeDlg.close();
#else
                qApp->quit();
#endif
            }
        } else
        {
#ifndef UNITTEST
            qApp->quit();
#endif
        }
    });

    //设置按钮信号
    connect(m_actionSettings, &QAction::triggered, this, &CMainWindow::slotPopupSettingsDialog);
    //切换分辨率
    connect(&Settings::get(), SIGNAL(resolutionchanged(const QString &)), m_videoPre, SLOT(slotresolutionchanged(const QString &)));
    //拍照
    connect(m_videoPre, SIGNAL(takePicOnce()), this, SLOT(onTakePicOnce()));
    //拍照取消
    connect(m_videoPre, SIGNAL(takePicCancel()), this, SLOT(onTakePicCancel()));
    //拍照结束
    connect(m_videoPre, SIGNAL(takePicDone()), this, SLOT(onTakePicDone()));
    //录制结束
    connect(m_videoPre, SIGNAL(takeVdDone()), this, SLOT(onTakeVdDone()));
    //录制取消 （倒计时3秒内的）
    connect(m_videoPre, SIGNAL(takeVdCancel()), this, SLOT(onTakeVdCancel()));
    //录制关机/休眠阻塞
    connect(m_videoPre, SIGNAL(updateBlockSystem(bool)), this, SLOT(updateBlockSystem(bool)));
    //没有相机了，结束拍照、录制
    connect(m_videoPre, SIGNAL(noCam()), this, SLOT(onNoCam()));
    //相机被抢占了，结束拍照、录制
    connect(m_videoPre, SIGNAL(noCamAvailable()), this, SLOT(onNoCam()));
    //设备切换信号
    connect(pSelectBtn, SIGNAL(clicked()), m_videoPre, SLOT(changeDev()));
    //设置新的分辨率
    connect(m_videoPre, SIGNAL(sigDeviceChange()), &Settings::get(), SLOT(setNewResolutionList()));
    //标题栏图片按钮
    connect(m_pTitlePicBtn, SIGNAL(clicked()), this, SLOT(onTitlePicBtn()));
    //标题栏视频按钮
    connect(m_pTitleVdBtn, SIGNAL(clicked()), this, SLOT(onTitleVdBtn()));
    //主题变换
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, &CMainWindow::onThemeChange);
}

void CMainWindow::initThumbnails()
{
    m_thumbnail = new ThumbnailsBar(this);
    m_rightbtnmenu = new QMenu(this);//添加右键打开文件夹功能
    m_actOpenfolder = new QAction(this);

    m_thumbnail->setObjectName("thumbnail");
    m_thumbnail->move(0, height() - 10);
    m_thumbnail->setFixedHeight(LAST_BUTTON_HEIGHT + LAST_BUTTON_SPACE * 2);
    m_videoPre->setThumbnail(m_thumbnail);
    m_rightbtnmenu->setObjectName("rightbtnmenu");
    m_actOpenfolder->setText(tr("Open folder"));
    m_rightbtnmenu->addAction(m_actOpenfolder);
    m_thumbnail->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_thumbnail, &DLabel::customContextMenuRequested, this, [ = ](QPoint pos) {
        Q_UNUSED(pos);
        m_rightbtnmenu->exec(QCursor::pos());
    });

    //右键菜单打开文件
    connect(m_actOpenfolder, &QAction::triggered, this, [ = ] {
        QString save_path = Settings::get().generalOption("last_open_pic_path").toString();

        if (save_path.isEmpty())
            save_path = Settings::get().getOption("base.save.picdatapath").toString();

        if (save_path.size() && save_path[0] == '~')
            save_path.replace(0, 1, QDir::homePath());

        if (!QFileInfo(save_path).exists())
        {
            QDir d;
            d.mkpath(save_path);
        }

        Dtk::Widget::DDesktopServices::showFolder(save_path);
    });

    m_thumbnail->setVisible(true);
    m_thumbnail->show();
    m_thumbnail->m_nMaxWidth = CMainWindow::minWindowWidth;
}

void CMainWindow::initThumbnailsConn()
{
    //系统文件夹变化信号
    connect(&m_fileWatcher, SIGNAL(directoryChanged(const QString &)), m_thumbnail, SLOT(onFoldersChanged(const QString &)));
    //系统文件变化信号
    connect(&m_fileWatcher, SIGNAL(fileChanged(const QString &)), m_thumbnail, SLOT(onFoldersChanged(const QString &)));//待测试
    //增删文件修改界面
    connect(m_thumbnail, SIGNAL(fitToolBar()), this, SLOT(onFitToolBar()));
    //修改标题栏按钮状态
    connect(m_thumbnail, SIGNAL(enableTitleBar(int)), this, SLOT(onEnableTitleBar(int)));
    //录像信号
    connect(m_thumbnail, SIGNAL(takeVd()), m_videoPre, SLOT(onTakeVideo()));
    //禁用设置
    connect(m_thumbnail, SIGNAL(enableSettings(bool)), this, SLOT(onEnableSettings(bool)));
    //拍照信号--显示倒计时
    connect(m_thumbnail, SIGNAL(takePic(bool)), m_videoPre, SLOT(onTakePic(bool)));
    //传递文件名，在拍照录制开始的时候，创建的文件不用于更新缩略图
    connect(m_videoPre, SIGNAL(filename(QString)), m_thumbnail, SLOT(onFileName(QString)));
}

void CMainWindow::setSelBtnHide()
{
    pSelectBtn->hide();
    pSelectBtn->setFocusPolicy(Qt::ClickFocus);
}

void CMainWindow::setSelBtnShow()
{
    pSelectBtn->show();
    pSelectBtn->setFocusPolicy(Qt::TabFocus);
}

void CMainWindow::setupTitlebar()
{
    m_titlemenu = new DMenu(this);
    m_actionSettings = new QAction(tr("Settings"), this);

    titlebar()->setObjectName("TitleBar");
    m_titlemenu->setObjectName("TitleMenu");
    m_actionSettings->setObjectName("SettingAction");
    m_titlemenu->addAction(m_actionSettings);
    titlebar()->setMenu(m_titlemenu);
    titlebar()->setParent(this);
}

void CMainWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    int width = this->width();

    if (m_thumbnail) {
        m_thumbnail->m_nMaxWidth = width;
        m_thumbnail->widthChanged();
        onFitToolBar();
    }

    if (m_videoPre)
        m_videoPre->update();
}

void CMainWindow::closeEvent(QCloseEvent *event)
{
    if (m_videoPre->getCapStatus()) {
        CloseDialog closeDlg(this, tr("Video recording is in progress. Close the window?"));
        closeDlg.setObjectName("closeDlg");
#ifdef UNITTEST
        closeDlg.show();
        event->ignore();
        closeDlg.close();
#else
        int ret = closeDlg.exec();

        switch (ret) {
        case 0:
            event->ignore();
            break;
        case 1:
            m_videoPre->onEndBtnClicked();
            event->accept();
            break;
        default:
            event->ignore();
            break;
        }
#endif
    }
}

void CMainWindow::changeEvent(QEvent *event)
{
    Q_UNUSED(event);

    if (windowState() == Qt::WindowMinimized)
        set_capture_pause(1);
    else if (windowState() == (Qt::WindowMinimized | Qt::WindowMaximized))
        set_capture_pause(1);
    else if (isVisible())
        set_capture_pause(0);
}

void CMainWindow::onFitToolBar()
{
    if (m_thumbnail) {
        int n = m_thumbnail->m_hBox->count();
        int nWidth = 0;

        if (n <= 0) {
            nWidth = LAST_BUTTON_SPACE * 2 + LAST_BUTTON_WIDTH;
            m_thumbnail->m_showVdTime->hide();
        } else {

            if (DataManager::instance()->getvideoCount() <= 0) {
                m_thumbnail->m_showVdTime->hide();
                nWidth = n * THUMBNAIL_WIDTH + ITEM_SPACE * (n - 1) + LAST_BUTTON_SPACE * 3 + LAST_BUTTON_WIDTH;
            } else {
                m_thumbnail->m_showVdTime->show();
                nWidth = n * THUMBNAIL_WIDTH + ITEM_SPACE * (n - 1) + LAST_BUTTON_SPACE * 4 + LAST_BUTTON_WIDTH + VIDEO_TIME_WIDTH;
            }

            if (DataManager::instance()->m_setIndex.size() >= 1)
                nWidth += DataManager::instance()->m_setIndex.size() * (SELECTED_WIDTH - THUMBNAIL_WIDTH);
            else
                nWidth += SELECTED_WIDTH - THUMBNAIL_WIDTH;

        }
        qDebug() << "onFitToolBar" << nWidth;
        m_thumbnail->resize(/*qMin(this->width(), nWidth)*/nWidth, THUMBNAIL_HEIGHT + 30);
        m_thumbnail->m_hBox->setSpacing(ITEM_SPACE);
        m_thumbnail->move((width() - m_thumbnail->width()) / 2,
                          height() - m_thumbnail->height() - 5);
    }
}

void CMainWindow::onEnableTitleBar(int nType)
{
    //1、禁用标题栏视频；2、禁用标题栏拍照；3、恢复标题栏视频；4、恢复标题栏拍照
    switch (nType) {
    case 1:
        m_pTitleVdBtn->setEnabled(false);
        pSelectBtn->setEnabled(false);
        break;
    case 2:
        m_pTitlePicBtn->setEnabled(false);
        pSelectBtn->setEnabled(false);
        break;
    case 3:
        m_pTitleVdBtn->setEnabled(true);
        pSelectBtn->setEnabled(true);
        break;
    case 4:
        m_pTitlePicBtn->setEnabled(true);
        pSelectBtn->setEnabled(true);
        break;
    default:
        break;
    }
}

void CMainWindow::menuItemInvoked(QAction *action)
{
    Q_UNUSED(action);
}

void CMainWindow::onTitlePicBtn()
{
    if (m_nActTpye == ActTakePic)
        return;

    m_nActTpye = ActTakePic;
    //切换标题栏拍照按钮颜色
    DPalette pa = m_pTitlePicBtn->palette();
    QColor clo("#0081FF");
    pa.setColor(DPalette::Dark, clo);
    pa.setColor(DPalette::Light, clo);
    pa.setColor(DPalette::Button, clo);
    m_pTitlePicBtn->setPalette(pa);
    QIcon iconPic(":/images/icons/light/button/photograph.svg");
    m_pTitlePicBtn->setIcon(iconPic);
    //切换标题栏视频按钮颜色
    DPalette paVd = m_pTitleVdBtn->palette();
    QColor cloVd("#000000");
    cloVd.setAlpha(20);
    paVd.setColor(DPalette::Dark, cloVd);
    paVd.setColor(DPalette::Light, cloVd);
    paVd.setColor(DPalette::Button, cloVd);
    m_pTitleVdBtn->setPalette(paVd);

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::UnknownType
            || DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        QIcon iconVd(":/images/icons/light/record video.svg");
        m_pTitleVdBtn->setIcon(iconVd);
    } else {
        QIcon iconVd(":/images/icons/dark/button/record video_dark.svg");
        m_pTitleVdBtn->setIcon(iconVd);
    }

    m_thumbnail->ChangeActType(m_nActTpye);
    SettingPathsave();
}

void CMainWindow::onTitleVdBtn()
{
    if (m_nActTpye == ActTakeVideo)
        return;

    m_nActTpye = ActTakeVideo;
    //切换标题栏视频按钮颜色
    DPalette paPic = m_pTitleVdBtn->palette();
    QColor cloPic("#0081FF");
    paPic.setColor(DPalette::Dark, cloPic);
    paPic.setColor(DPalette::Light, cloPic);
    paPic.setColor(DPalette::Button, cloPic);
    m_pTitleVdBtn->setPalette(paPic);
    QIcon iconVd(":/images/icons/light/button/transcribe.svg");
    m_pTitleVdBtn->setIcon(iconVd);
    //切换标题栏拍照按钮颜色
    DPalette paVd = m_pTitlePicBtn->palette();
    QColor cloVd("#000000");
    cloVd.setAlpha(20);
    paVd.setColor(DPalette::Dark, cloVd);
    paVd.setColor(DPalette::Light, cloVd);
    paVd.setColor(DPalette::Button, cloVd);
    m_pTitlePicBtn->setPalette(paVd);

    if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::UnknownType
            || DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::LightType) {
        QIcon iconVd(":/images/icons/light/photograph.svg");
        m_pTitlePicBtn->setIcon(iconVd);
    } else {
        QIcon iconPic(":/images/icons/dark/button/photograph_dark.svg");
        m_pTitlePicBtn->setIcon(iconPic);
    }

    m_thumbnail->ChangeActType(m_nActTpye);
    SettingPathsave();
}

void CMainWindow::onSettingsDlgClose()
{
    /**********************************************/
    if (QDir(Settings::get().getOption("base.save.vddatapath").toString()).exists() == false)
        CMainWindow::lastVdFileName = QDir::homePath() + QDir::separator() + "Videos" + QDir::separator() + QObject::tr("Camera");

    if (QDir(Settings::get().getOption("base.save.picdatapath").toString()).exists() == false)
        CMainWindow::lastPicFileName = QDir::homePath() + QDir::separator() + "Pictures" + QDir::separator() + QObject::tr("Camera");

    if (CMainWindow::lastVdFileName.size() && CMainWindow::lastVdFileName[0] == '~')
        CMainWindow::lastVdFileName.replace(0, 1, QDir::homePath());

    if (CMainWindow::lastPicFileName.size() && CMainWindow::lastPicFileName[0] == '~')
        CMainWindow::lastPicFileName.replace(0, 1, QDir::homePath());

    CMainWindow::lastVdFileName = Settings::get().getOption("base.save.vddatapath").toString();
    CMainWindow::lastPicFileName = Settings::get().getOption("base.save.picdatapath").toString();

    if (m_nActTpye == ActTakeVideo)
        m_videoPre->setSaveFolder(CMainWindow::lastVdFileName);
    else
        m_videoPre->setSaveFolder(CMainWindow::lastPicFileName);

    //关闭设置时，添加保存路径下图片和视频的缩略图
    m_fileWatcher.addPath(CMainWindow::lastVdFileName);
    m_thumbnail->addPath(CMainWindow::lastVdFileName);
    m_fileWatcher.addPath(CMainWindow::lastPicFileName);
    m_thumbnail->addPath(CMainWindow::lastPicFileName);

    int nContinuous = Settings::get().getOption("photosetting.photosnumber.takephotos").toInt();
    int nDelayTime = Settings::get().getOption("photosetting.photosdelay.photodelays").toInt();
    bool soundphoto = Settings::get().getOption("photosetting.audiosetting.soundswitchbtn").toBool();

    /**********************************************/
    switch (nContinuous) {
    case 1:
        nContinuous = 4;
        break;
    case 2:
        nContinuous = 10;
        break;
    default:
        nContinuous = 1;
        break;
    }
    switch (nDelayTime) {
    case 1:
        nDelayTime = 3;
        break;
    case 2:
        nDelayTime = 6;
        break;
    default:
        nDelayTime = 0;
        break;
    }

    if (soundphoto)
        set_takeing_photo_sound(1);
    else
        set_takeing_photo_sound(0);

    m_videoPre->setInterval(nDelayTime);
    m_videoPre->setContinuous(nContinuous);
    Settings::get().settings()->sync();
}

void CMainWindow::onEnableSettings(bool bTrue)
{
    m_actionSettings->setEnabled(bTrue);
}

void CMainWindow::onTakePicDone()
{
    qDebug() << "onTakePicDone";
    onEnableTitleBar(3); //恢复按钮状态
    onEnableSettings(true);
    m_thumbnail->m_nStatus = STATNULL;
    m_thumbnail->setBtntooltip();
    QString strPath = m_videoPre->m_imgPrcThread->m_strPath;
    m_thumbnail->addFile(strPath);
}

void CMainWindow::onTakePicOnce()
{
    qDebug() << "onTakePicOnce";
    m_thumbnail->addFile(m_videoPre->m_imgPrcThread->m_strPath);
}

void CMainWindow::onTakePicCancel()
{
    onEnableTitleBar(3); //恢复按钮状态
    onEnableSettings(true);
    m_thumbnail->m_nStatus = STATNULL;
    m_thumbnail->setBtntooltip();
}

void CMainWindow::onTakeVdDone()
{
    onEnableTitleBar(4); //恢复按钮状态
    m_thumbnail->m_nStatus = STATNULL;
    m_thumbnail->show();
    onEnableSettings(true);

    QTimer::singleShot(200, this, [ = ] {
        QString strFileName = m_videoPre->getFolder() + "/" + DataManager::instance()->getstrFileName();
        QFile file(strFileName);

        if (!file.exists())
            usleep(200000);

        m_thumbnail->addFile(strFileName);
    });
}

void CMainWindow::onTakeVdCancel()   //保存视频完成，通过已有的文件检测实现缩略图恢复，这里不需要额外处理
{
    onEnableTitleBar(4); //恢复按钮状态
    m_thumbnail->m_nStatus = STATNULL;
    m_thumbnail->show();
    onEnableSettings(true);
}

void CMainWindow::onThemeChange(DGuiApplicationHelper::ColorType type)
{
    if (type == DGuiApplicationHelper::UnknownType || type == DGuiApplicationHelper::LightType) {
        pSelectBtn->setIconSize(QSize(37, 37));
        pSelectBtn->setIcon(QIcon(":/images/icons/light/button/Switch camera.svg"));

        if (m_nActTpye == ActTakePic)
            m_pTitleVdBtn->setIcon(QIcon(":/images/icons/light/record video.svg"));
        else
            m_pTitlePicBtn->setIcon(QIcon(":/images/icons/light/photograph.svg"));
    }

    if (type == DGuiApplicationHelper::DarkType) {
        pSelectBtn->setIcon(QIcon(":/images/icons/dark/button/Switch camera_dark.svg"));

        if (m_nActTpye == ActTakePic)
            m_pTitleVdBtn->setIcon(QIcon(":/images/icons/dark/button/record video_dark.svg"));
        else
            m_pTitlePicBtn->setIcon(QIcon(":/images/icons/dark/button/photograph_dark.svg"));
    }
}

void CMainWindow::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Shift) {
        qDebug() << "shift pressed";
        DataManager::instance()->setbMultiSlt(true);
        DataManager::instance()->m_setIndex.insert(DataManager::instance()->getindexNow());
    }
}

void CMainWindow::keyReleaseEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Shift) {
        qDebug() << "shift released";
        DataManager::instance()->setbMultiSlt(false);
    }
}
void CMainWindow::SettingPathsave()
{
    if (m_nActTpye == ActTakeVideo)
        m_videoPre->setSaveFolder(CMainWindow::lastVdFileName);
    else
        m_videoPre->setSaveFolder(CMainWindow::lastPicFileName);

}