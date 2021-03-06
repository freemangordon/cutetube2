/*
 * Copyright (C) 2015 Stuart Howarth <showarth@marxoft.co.uk>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "youtubevideoswindow.h"
#include "clipboard.h"
#include "imagecache.h"
#include "listview.h"
#include "settings.h"
#include "videodelegate.h"
#include "videoplaybackwindow.h"
#include "youtube.h"
#include "youtubedownloaddialog.h"
#include "youtubeplaybackdialog.h"
#include "youtubeplaylistdialog.h"
#include "youtubevideowindow.h"
#include <qyoutube/urls.h>
#include <QLabel>
#include <QActionGroup>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QMaemo5InformationBox>

YouTubeVideosWindow::YouTubeVideosWindow(StackedWindow *parent) :
    StackedWindow(parent),
    m_model(new YouTubeVideoModel(this)),
    m_cache(new ImageCache),
    m_view(new ListView(this)),
    m_delegate(new VideoDelegate(m_cache, YouTubeVideoModel::DateRole, YouTubeVideoModel::DurationRole,
                                 YouTubeVideoModel::ThumbnailUrlRole, YouTubeVideoModel::TitleRole,
                                 YouTubeVideoModel::UsernameRole, m_view)),
    m_viewGroup(new QActionGroup(this)),
    m_listAction(new QAction(tr("List"), this)),
    m_gridAction(new QAction(tr("Grid"), this)),
    m_reloadAction(new QAction(tr("Reload"), this)),
    m_contextMenu(new QMenu(this)),
    m_downloadAction(new QAction(tr("Download"), this)),
    m_shareAction(new QAction(tr("Copy URL"), this)),
    m_favouriteAction(0),
    m_watchLaterAction(0),
    m_playlistAction(0),
    m_label(new QLabel(QString("<p align='center'; style='font-size: 40px; color: %1'>%2</p>")
                              .arg(palette().color(QPalette::Mid).name()).arg(tr("No videos found")), this))
{
    setWindowTitle(tr("Videos"));
    setCentralWidget(new QWidget);
    
    m_view->setModel(m_model);
    m_view->setItemDelegate(m_delegate);
    m_view->setContextMenuPolicy(Qt::CustomContextMenu);
    
    m_listAction->setCheckable(true);
    m_listAction->setChecked(true);
    m_gridAction->setCheckable(true);
    m_viewGroup->setExclusive(true);
    m_viewGroup->addAction(m_listAction);
    m_viewGroup->addAction(m_gridAction);
    m_reloadAction->setEnabled(false);
    
    m_contextMenu->addAction(m_downloadAction);
    m_contextMenu->addAction(m_shareAction);
    
    m_label->hide();
    
    m_layout = new QVBoxLayout(centralWidget());
    m_layout->addWidget(m_view);
    m_layout->addWidget(m_label);
    m_layout->setContentsMargins(0, 0, 0, 0);
    
    menuBar()->addAction(m_listAction);
    menuBar()->addAction(m_gridAction);
    menuBar()->addAction(m_reloadAction);
    
    connect(m_model, SIGNAL(statusChanged(QYouTube::ResourcesRequest::Status)), this,
            SLOT(onModelStatusChanged(QYouTube::ResourcesRequest::Status)));
    connect(m_cache, SIGNAL(imageReady()), this, SLOT(onImageReady()));
    connect(m_view, SIGNAL(activated(QModelIndex)), this, SLOT(showVideo(QModelIndex)));
    connect(m_view, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(m_delegate, SIGNAL(thumbnailClicked(QModelIndex)), this, SLOT(playVideo(QModelIndex)));
    connect(m_listAction, SIGNAL(triggered()), this, SLOT(enableListMode()));
    connect(m_gridAction, SIGNAL(triggered()), this, SLOT(enableGridMode()));
    connect(m_reloadAction, SIGNAL(triggered()), m_model, SLOT(reload()));
    connect(m_downloadAction, SIGNAL(triggered()), this, SLOT(downloadVideo()));
    connect(m_shareAction, SIGNAL(triggered()), this, SLOT(shareVideo()));
    
    if (!YouTube::instance()->userId().isEmpty()) {
        if ((YouTube::instance()->hasScope(QYouTube::READ_WRITE_SCOPE))
            || (YouTube::instance()->hasScope(QYouTube::FORCE_SSL_SCOPE))) {
            m_favouriteAction = new QAction(this);
            m_watchLaterAction = new QAction(tr("Watch later"), this);
            m_playlistAction = new QAction(tr("Add to playlist"), this);
            m_contextMenu->addAction(m_favouriteAction);
            m_contextMenu->addAction(m_watchLaterAction);
            m_contextMenu->addAction(m_playlistAction);
            connect(m_favouriteAction, SIGNAL(triggered()), this, SLOT(setVideoFavourite()));
            connect(m_watchLaterAction, SIGNAL(triggered()), this, SLOT(watchVideoLater()));
            connect(m_playlistAction, SIGNAL(triggered()), this, SLOT(addVideoToPlaylist()));
        }
    }
    
    if (Settings::instance()->defaultViewMode() == "grid") {
        m_gridAction->trigger();
    }
}

YouTubeVideosWindow::~YouTubeVideosWindow() {
    delete m_cache;
    m_cache = 0;
}

void YouTubeVideosWindow::list(const QString &resourcePath, const QStringList &part, const QVariantMap &filters,
                               const QVariantMap &params) {
    m_model->list(resourcePath, part, filters, params);
}

void YouTubeVideosWindow::enableGridMode() {
    m_delegate->setGridMode(true);
    m_view->setFlow(QListView::LeftToRight);
    m_view->setWrapping(true);
    Settings::instance()->setDefaultViewMode("grid");
}

void YouTubeVideosWindow::enableListMode() {
    m_delegate->setGridMode(false);
    m_view->setFlow(QListView::TopToBottom);
    m_view->setWrapping(false);
    Settings::instance()->setDefaultViewMode("list");
}

void YouTubeVideosWindow::addVideoToPlaylist() {
    if (isBusy()) {
        return;
    }
    
    if (YouTubeVideo *video = m_model->get(m_view->currentIndex().row())) {
        YouTubePlaylistDialog *dialog = new YouTubePlaylistDialog(video, this);
        dialog->open();
    }
}

void YouTubeVideosWindow::downloadVideo() {
    if (isBusy()) {
        return;
    }
    
    if (m_view->currentIndex().isValid()) {
        QString id = m_view->currentIndex().data(YouTubeVideoModel::IdRole).toString();
        QString title = m_view->currentIndex().data(YouTubeVideoModel::TitleRole).toString();
        
        YouTubeDownloadDialog *dialog = new YouTubeDownloadDialog(id, title, this);
        dialog->open();
    }
}

void YouTubeVideosWindow::playVideo(const QModelIndex &index) {
    if (isBusy()) {
        return;
    }
    
    if (Settings::instance()->videoPlayer() == "cutetube") {
        if (YouTubeVideo *video = m_model->get(index.row())) {
            VideoPlaybackWindow *window = new VideoPlaybackWindow(this);
            window->show();
            window->addVideo(video);
        }
    }
    else {
        QString id = index.data(YouTubeVideoModel::IdRole).toString();
        QString title = index.data(YouTubeVideoModel::TitleRole).toString();
    
        YouTubePlaybackDialog *dialog = new YouTubePlaybackDialog(id, title, this);
        dialog->open();
    }
}

void YouTubeVideosWindow::setVideoFavourite() {
    if (isBusy()) {
        return;
    }
    
    if (YouTubeVideo *video = m_model->get(m_view->currentIndex().row())) {
        connect(video, SIGNAL(statusChanged(QYouTube::ResourcesRequest::Status)),
                this, SLOT(onVideoUpdateStatusChanged(QYouTube::ResourcesRequest::Status)));
        
        if (video->isFavourite()) {
            video->unfavourite();
        }
        else {
            video->favourite();
        }
    }
}

void YouTubeVideosWindow::shareVideo() {
    if (const YouTubeVideo *video = m_model->get(m_view->currentIndex().row())) {
        Clipboard::instance()->setText(video->url().toString());
        QMaemo5InformationBox::information(this, tr("URL copied to clipboard"));
    }
}

void YouTubeVideosWindow::showVideo(const QModelIndex &index) {
    if (isBusy()) {
        return;
    }
    
    if (const YouTubeVideo *video = m_model->get(index.row())) {
        YouTubeVideoWindow *window = new YouTubeVideoWindow(video, this);
        window->show();
    }
}

void YouTubeVideosWindow::watchVideoLater() {
    if (isBusy()) {
        return;
    }
    
    if (YouTubeVideo *video = m_model->get(m_view->currentIndex().row())) {
        connect(video, SIGNAL(statusChanged(QYouTube::ResourcesRequest::Status)),
                this, SLOT(onVideoUpdateStatusChanged(QYouTube::ResourcesRequest::Status)));
        video->watchLater();
    }
}

void YouTubeVideosWindow::showContextMenu(const QPoint &pos) {
    if ((!isBusy()) && (m_view->currentIndex().isValid())) {
        if (m_favouriteAction) {
            m_favouriteAction->setText(m_view->currentIndex().data(YouTubeVideoModel::FavouriteRole).toBool()
                                       ? tr("Unfavourite") : tr("Favourite"));
        }
        
        m_contextMenu->popup(pos, m_downloadAction);
    }
}

void YouTubeVideosWindow::onImageReady() {
    m_view->viewport()->update(m_view->viewport()->rect());
}

void YouTubeVideosWindow::onModelStatusChanged(QYouTube::ResourcesRequest::Status status) {
    switch (status) {
    case QYouTube::ResourcesRequest::Loading:
        showProgressIndicator();
        m_label->hide();
        m_view->show();
        m_reloadAction->setEnabled(false);
        return;
    case QYouTube::ResourcesRequest::Failed:
        QMessageBox::critical(this, tr("Error"), m_model->errorString());
        break;
    default:
        break;
    }
    
    hideProgressIndicator();
    m_reloadAction->setEnabled(true);
    
    if (m_model->rowCount() == 0) {
        m_view->hide();
        m_label->show();
    }
}

void YouTubeVideosWindow::onVideoUpdateStatusChanged(QYouTube::ResourcesRequest::Status status) {
    const YouTubeVideo *video = qobject_cast<YouTubeVideo*>(sender());
    
    if (!video) {
        return;
    }
    
    switch (status) {
    case QYouTube::ResourcesRequest::Loading:
        showProgressIndicator();
        return;
    case QYouTube::ResourcesRequest::Failed:
        QMessageBox::critical(this, tr("Error"), video->errorString());
        break;
    default:
        break;
    }
    
    hideProgressIndicator();
    disconnect(video, SIGNAL(statusChanged(QYouTube::ResourcesRequest::Status)),
               this, SLOT(onVideoUpdateStatusChanged(QYouTube::ResourcesRequest::Status)));
}
