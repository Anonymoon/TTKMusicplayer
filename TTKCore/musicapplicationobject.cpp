#include "musicapplicationobject.h"
#ifdef Q_OS_WIN
# include <Windows.h>
# include <Dbt.h>
#endif
#include "musicmobiledeviceswidget.h"
#include "musicaudiorecorderwidget.h"
#include "musictimerwidget.h"
#include "musictimerautoobject.h"
#include "musicmessagebox.h"
#include "musicequalizerdialog.h"
#include "musicconnectionpool.h"
#include "musicsettingmanager.h"
#include "musicregeditmanager.h"
#include "musicmobiledevicesthread.h"

#include <QPropertyAnimation>
#include <QApplication>
#include <QDesktopWidget>

MusicApplicationObject::MusicApplicationObject(QObject *parent)
    : QObject(parent), m_mobileDevices(nullptr)
{
    m_supperClass = MStatic_cast(QWidget*, parent);
    QWidget *widget = QApplication::desktop();
    m_supperClass->move( (widget->width() - m_supperClass->width())/2,
                         (widget->height() - m_supperClass->height())/2 );
    M_SETTING->setValue(MusicSettingManager::ScreenSize, widget->size());

    windowStartAnimationOpacity();
    m_musicTimerAutoObj = new MusicTimerAutoObject(this);
    connect(m_musicTimerAutoObj, SIGNAL(setPlaySong(int)), parent,
                                 SLOT(setPlaySongChanged(int)));
    connect(m_musicTimerAutoObj, SIGNAL(setStopSong()), parent,
                                 SLOT(setStopSongChanged()));
#ifdef Q_OS_UNIX
    m_mobileDevicesLinux = new MusicMobileDevicesThread(this);
    connect(m_mobileDevicesLinux, SIGNAL(devicesChanged(bool)), SLOT(musicDevicesLinuxChanged(bool)));
    m_mobileDevicesLinux->start();
#elif defined Q_OS_WIN
    m_mobileDevicesLinux = nullptr;
#endif

    m_setWindowToTop = false;
    M_CONNECTION->setValue("MusicApplicationObject", this);
    M_CONNECTION->poolConnect("MusicApplicationObject", "MusicApplication");
    M_CONNECTION->poolConnect("MusicApplicationObject", "MusicEnhancedWidget");

    musicToolSetsParameter();
}

MusicApplicationObject::~MusicApplicationObject()
{
    delete m_mobileDevicesLinux;
    delete m_mobileDevices;
    delete m_musicTimerAutoObj;
    delete m_animation;
}

void MusicApplicationObject::getParameterSetting()
{
#ifdef Q_OS_WIN
    if(M_SETTING->value(MusicSettingManager::FileAssociationChoiced).toInt())
    {
        MusicRegeditManager regeditManager;
        regeditManager.setMusicRegeditAssociateFileIcon();
    }
#endif
}

void MusicApplicationObject::windowStartAnimationOpacity()
{
    m_animation = new QPropertyAnimation(m_supperClass, "windowOpacity");
    m_animation->setDuration(1000);
    m_animation->setStartValue(0);
    m_animation->setEndValue(1);
    m_animation->start();
}

void MusicApplicationObject::windowCloseAnimationOpacity()
{
    m_animation->stop();
    m_animation->setDuration(1000);
    m_animation->setStartValue(1);
    m_animation->setEndValue(0);
    m_animation->start();
    QTimer::singleShot(1000, qApp, SLOT(quit()));
}

#if defined(Q_OS_WIN)
#  ifdef MUSIC_QT_5
void MusicApplicationObject::nativeEvent(const QByteArray &,
                                         void *message, long *)
{
    MSG* msg = MReinterpret_cast(MSG*, message);
#  else
void MusicApplicationObject::winEvent(MSG *msg, long *)
{
#  endif
    if(msg->message == WM_DEVICECHANGE)
    {
        PDEV_BROADCAST_HDR lpdb = (PDEV_BROADCAST_HDR)msg->lParam;
        switch(msg->wParam)
        {
            case DBT_DEVICETYPESPECIFIC:
                break;
            case DBT_DEVICEARRIVAL:
                if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
                {
                    PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                    if (lpdbv->dbcv_flags == 0)
                    {
                        DWORD unitmask = lpdbv ->dbcv_unitmask;
                        int i;
                        for(i = 0; i < 26; ++i)
                        {
                            if(unitmask & 0x1)
                                break;
                            unitmask = unitmask >> 1;
                        }
                        M_LOGGER << "USB_Arrived and The USBDisk is: "
                                 << (char)(i + 'A') << LOG_END;
                        delete m_mobileDevices;
                        m_mobileDevices = new MusicMobileDevicesWidget;
                        m_mobileDevices->show();
                    }
                }
                break;
            case DBT_DEVICEREMOVECOMPLETE:
                if(lpdb->dbch_devicetype == DBT_DEVTYP_VOLUME)
                {
                    PDEV_BROADCAST_VOLUME lpdbv = (PDEV_BROADCAST_VOLUME)lpdb;
                    if (lpdbv -> dbcv_flags == 0)
                    {
                        M_LOGGER << "USB_remove" << LOG_END;
                        delete m_mobileDevices;
                        m_mobileDevices = nullptr;
                    }
                }
                break;
            default: break;
        }
    }
}
#endif

void MusicApplicationObject::musicAboutUs()
{
    MusicMessageBox message;
    message.setText(tr("TTK Music Player") + QString("\n\n") +
                    tr("Directed By Greedysky") +
                    QString("\nCopyright© 2014-2016") +
                    QString("\nMail:Greedysky@163.com"));
    message.exec();
}

void MusicApplicationObject::musicAudioRecorder()
{
    MusicAudioRecorderWidget recorder;
    recorder.exec();
}

void MusicApplicationObject::musicTimerWidget()
{
    MusicTimerWidget timer;
    QStringList list;
    emit getCurrentPlayList(list);
    timer.setSongStringList(list);
    timer.exec();
}

void MusicApplicationObject::musicSetWindowToTop()
{
    m_setWindowToTop = !m_setWindowToTop;
    Qt::WindowFlags flags = m_supperClass->windowFlags();
    m_supperClass->setWindowFlags( m_setWindowToTop ?
                  (flags | Qt::WindowStaysOnTopHint) :
                  (flags & ~Qt::WindowStaysOnTopHint) );
    m_supperClass->show();
}

void MusicApplicationObject::musicToolSetsParameter()
{
    m_musicTimerAutoObj->runTimerAutoConfig();
}

void MusicApplicationObject::musicSetEqualizer()
{
    if(M_SETTING->value(MusicSettingManager::EnhancedMusicChoiced).toInt() != 0)
    {
        MusicMessageBox message;
        message.setText(tr("we are opening the magic sound, if you want to close?"));
        if(message.exec())
        {
            return;
        }
        emit enhancedMusicChanged(0);
    }
    MusicEqualizerDialog eq;
    eq.exec();
}

void MusicApplicationObject::musicDevicesLinuxChanged(bool state)
{
    delete m_mobileDevices;
    m_mobileDevices = nullptr;
    if(state)
    {
        m_mobileDevices = new MusicMobileDevicesWidget;
        m_mobileDevices->show();
    }
}