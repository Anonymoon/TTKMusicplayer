#include "musicdownloadwidget.h"
#include "ui_musicdownloadwidget.h"
#include "musicbgthememanager.h"
#include "musicuiobject.h"
#include "musicsettingmanager.h"
#include "musicnetworkthread.h"
#include "musicmydownloadrecordobject.h"
#include "musicsongdownloadthread.h"
#include "musictextdownloadthread.h"
#include "musicdatadownloadthread.h"
#include "musicdata2downloadthread.h"
#include "musicbgthemedownload.h"

#include <QFileDialog>
#include <QLabel>

MusicDownloadTableItem::MusicDownloadTableItem(QWidget *parent)
    : QWidget(parent)
{
    m_infomation = new QLabel(this);
    m_icon = new QLabel(this);
    m_text = new QLabel(this);

    m_text->setGeometry(0, 0, 60, 25);
    m_icon->setGeometry(70, 0, 30, 25);
    m_infomation->setGeometry(170, 0, 150, 25);
}

MusicDownloadTableItem::~MusicDownloadTableItem()
{
    delete m_infomation;
    delete m_icon;
    delete m_text;
}

void MusicDownloadTableItem::setIcon(const QString &name)
{
    m_icon->setPixmap(QPixmap(name).scaled(28, 18));
}

void MusicDownloadTableItem::setInfomation(const QString &info)
{
    m_infomation->setText(info);
}

void MusicDownloadTableItem::setText(const QString &text)
{
    m_text->setText(text);
}



MusicDownloadTableWidget::MusicDownloadTableWidget(QWidget *parent)
    : MusicAbstractTableWidget(parent)
{
    setColumnCount(1);
    QHeaderView *headerview = horizontalHeader();
    headerview->resizeSection(0, 400);
    setTransparent(255);
}

MusicDownloadTableWidget::~MusicDownloadTableWidget()
{
    clearAllItems();
}

void MusicDownloadTableWidget::clearAllItems()
{
    while(!m_items.isEmpty())
    {
        delete m_items.takeFirst();
    }
    MusicAbstractTableWidget::clear();
    setColumnCount(1);
}

void MusicDownloadTableWidget::createItem(const QString &type, const QString &info, const QString &icon)
{
    int index = rowCount();
    setRowCount(index + 1);
    setRowHeight(index, 25);
    setItem(index, 0, new QTableWidgetItem);

    MusicDownloadTableItem *item = new MusicDownloadTableItem(this);
    item->setIcon(icon);
    item->setInfomation(info);
    item->setText(type);
    m_items << item;

    setCellWidget(index, 0, item);
}


MusicDownloadWidget::MusicDownloadWidget(QWidget *parent)
    : MusicAbstractMoveDialog(parent),
      ui(new Ui::MusicDownloadWidget)
{
    ui->setupUi(this);
    ui->topTitleCloseButton->setIcon(QIcon(":/share/searchclosed"));
    ui->topTitleCloseButton->setStyleSheet(MusicUIObject::MToolButtonStyle03);
    ui->topTitleCloseButton->setCursor(QCursor(Qt::PointingHandCursor));
    ui->topTitleCloseButton->setToolTip(tr("Close"));

    m_downloadThread = new MusicDownLoadQueryThread(this);
    m_queryType = MusicQuery;

    connect(ui->pathChangedButton, SIGNAL(clicked()), SLOT(downloadDirSelected()));
    connect(m_downloadThread, SIGNAL(resolvedSuccess()), SLOT(queryAllFinished()));
    connect(ui->topTitleCloseButton, SIGNAL(clicked()), SLOT(close()));
    connect(ui->downloadButton, SIGNAL(clicked()), SLOT(startToDownload()));
}

MusicDownloadWidget::~MusicDownloadWidget()
{
    delete ui;
    delete m_downloadThread;
}

void MusicDownloadWidget::initWidget()
{
    QString path = M_SETTING->value(MusicSettingManager::DownloadMusicPathDirChoiced).toString();
    ui->downloadPathEdit->setText(path.isEmpty() ? MUSIC_DOWNLOAD_AL : path);
}

void MusicDownloadWidget::setSongName(const QString &name, QueryType type)
{
    initWidget();
    ui->downloadName->setText(QFontMetrics(font()).elidedText(name, Qt::ElideRight, 200));
    m_downloadThread->setQueryAllRecords(true);
    m_downloadThread->startSearchSong(m_queryType = type, name);
}

int MusicDownloadWidget::exec()
{
    QPixmap pix(M_BG_MANAGER->getMBackground());
    ui->background->setPixmap(pix.scaled( size() ));
    return MusicAbstractMoveDialog::exec();
}

void MusicDownloadWidget::queryAllFinished()
{
    if(m_queryType == MusicQuery)
    {
        queryAllFinishedMusic();
    }
    else if(m_queryType == MovieQuery)
    {
        queryAllFinishedMovie();
    }
}

void MusicDownloadWidget::queryAllFinishedMusic()
{
    MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        MusicSongAttributes attrs = musicSongInfos.first().m_songAttrs;
        foreach(MusicSongAttribute attr, attrs)
        {
            if(attr.m_bitrate == 32)         ///st
            {
                ui->viewArea->createItem(tr("ST"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                         .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                         QString(":/quality/stQuality"));
            }
            else if(attr.m_bitrate == 128)   ///sd
            {
                ui->viewArea->createItem(tr("SD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                         .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                         QString(":/quality/sdQuality"));
            }
            else if(attr.m_bitrate == 192)   ///hd
            {
                ui->viewArea->createItem(tr("HD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                         .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                         QString(":/quality/hdQuality"));
            }
            else if(attr.m_bitrate == 320)   ///sq
            {
                ui->viewArea->createItem(tr("SQ"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                         .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                         QString(":/quality/sqQuality"));
            }
            else if(attr.m_bitrate > 320)   ///cd
            {
                ui->viewArea->createItem(tr("CD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                         .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                         QString(":/quality/cdQuality"));
            }
        }
    }
}

void MusicDownloadWidget::queryAllFinishedMovie()
{
    MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        MusicSongAttributes attrs = musicSongInfos.first().m_songAttrs;
        foreach(MusicSongAttribute attr, attrs)
        {
            if(attr.m_bitrate == 500)      ///hd
            {
                ui->viewArea->createItem(tr("HD"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                         .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                         QString(":/quality/hdQuality"));
            }
            else if(attr.m_bitrate == 750) ///sq
            {
                ui->viewArea->createItem(tr("SQ"), QString("%1/%2KBPS/%3").arg(attr.m_size)
                                         .arg(attr.m_bitrate).arg(attr.m_format.toUpper()),
                                         QString(":/quality/sqQuality"));
            }
        }
    }
}

void MusicDownloadWidget::downloadDirSelected()
{
    QFileDialog dialog;
    dialog.setFileMode(QFileDialog::Directory );
    dialog.setViewMode(QFileDialog::Detail);
    if(dialog.exec())
    {
        QString path;
        if(!(path = dialog.directory().absolutePath()).isEmpty())
        {
            M_SETTING->setValue(MusicSettingManager::DownloadMusicPathDirChoiced, path);
            ui->downloadPathEdit->setText(path);
        }
    }
}

void MusicDownloadWidget::startToDownload()
{
    if(m_queryType == MusicQuery)
    {
        startToDownloadMusic();
    }
    else if(m_queryType == MovieQuery)
    {
        startToDownloadMovie();
    }
}

void MusicDownloadWidget::startToDownloadMusic()
{
    int bitrate = -1;
//    if(ui->radioButtonST->isChecked()) bitrate = 32;
//    else if(ui->radioButtonHD->isChecked()) bitrate = 128;
//    else if(ui->radioButtonSD->isChecked()) bitrate = 320;

    MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        MusicSongInfomation musicSongInfo = musicSongInfos.first();
        MusicSongAttributes musicAttrs = musicSongInfo.m_songAttrs;
        foreach(MusicSongAttribute musicAttr, musicAttrs)
        {
            if(musicAttr.m_bitrate == bitrate)
            {
                if(!M_NETWORK->isOnline())
                {
                    return;
                }
                QString musicSong = musicSongInfo.m_singerName + " - " + musicSongInfo.m_songName;
                QString downloadName = QString("%1%2.%3").arg(MUSIC_DOWNLOAD_AL)
                                                         .arg(musicSong).arg(musicAttr.m_format);
                ////////////////////////////////////////////////
                MusicDownloadRecord record;
                MusicMyDownloadRecordObject down(this);
                if(!down.readDownloadXMLConfig())
                {
                    return;
                }

                down.readDownloadConfig( record );
                record.m_names << musicSong;
                record.m_paths << QFileInfo(downloadName).absoluteFilePath();
                record.m_sizes << musicAttr.m_size;
                down.writeDownloadConfig( record );
                ////////////////////////////////////////////////

                MusicSongDownloadThread *downSong = new MusicSongDownloadThread( musicAttr.m_url,
                                                                                 downloadName, Download_Music, this);
                downSong->startToDownload();

                (new MusicTextDownLoadThread(musicSongInfo.m_lrcUrl, LRC_DOWNLOAD_AL +
                                             musicSong + LRC_FILE, Download_Lrc, this))->startToDownload();
                (new MusicData2DownloadThread(musicSongInfo.m_smallPicUrl, ART_DOWNLOAD_AL +
                                              musicSongInfo.m_singerName + SKN_FILE, Download_SmlBG, this))->startToDownload();
                ///download big picture
                (new MusicBgThemeDownload(musicSongInfo.m_singerName, musicSongInfo.m_singerName, this))->startToDownload();

                break;
            }
        }
    }
}

void MusicDownloadWidget::startToDownloadMovie()
{
    int bitrate = -1;
//    if(ui->radioButtonHD_MV->isChecked()) bitrate = 500;
//    else if(ui->radioButtonSD_MV->isChecked()) bitrate = 750;

    MusicSongInfomations musicSongInfos(m_downloadThread->getMusicSongInfos());
    if(!musicSongInfos.isEmpty())
    {
        MusicSongInfomation musicSongInfo = musicSongInfos.first();
        MusicSongAttributes musicAttrs = musicSongInfo.m_songAttrs;
        foreach(MusicSongAttribute musicAttr, musicAttrs)
        {
            if(musicAttr.m_bitrate == bitrate)
            {
                if(!M_NETWORK->isOnline())
                {
                    return;
                }
                MusicDataDownloadThread* download = new MusicDataDownloadThread(musicAttr.m_url,
                            QString("%1/%2 - %3.%4").arg(MOVIE_DOWNLOAD_AL).arg(musicSongInfo.m_singerName)
                                                    .arg(musicSongInfo.m_songName).arg(musicAttr.m_format), Download_Video, this);
                download->startToDownload();
                break;
            }
        }
    }
}
