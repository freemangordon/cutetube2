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

#include "youtubevideomodel.h"
#include "youtube.h"
#include "youtubeplaylist.h"
#ifdef CUTETUBE_DEBUG
#include <QDebug>
#endif

YouTubeVideoModel::YouTubeVideoModel(QObject *parent) :
    QAbstractListModel(parent),
    m_request(new QYouTube::ResourcesRequest(this)),
    m_contentRequest(0)
{
    m_roles[DateRole] = "date";
    m_roles[DescriptionRole] = "description";
    m_roles[DislikedRole] = "disliked";
    m_roles[DislikeCountRole] = "dislikeCount";
    m_roles[DurationRole] = "duration";
    m_roles[FavouriteRole] = "favourited";
    m_roles[FavouriteCountRole] = "favouriteCount";
    m_roles[FavouriteIdRole] = "favouriteId";
    m_roles[IdRole] = "id";
    m_roles[LargeThumbnailUrlRole] = "largeThumbnailUrl";
    m_roles[LikedRole] = "liked";
    m_roles[LikeCountRole] = "likeCount";
    m_roles[PlaylistItemIdRole] = "playlistItemId";
    m_roles[ThumbnailUrlRole] = "thumbnailUrl";
    m_roles[TitleRole] = "title";
    m_roles[UrlRole] = "url";
    m_roles[UserIdRole] = "userId";
    m_roles[UsernameRole] = "username";
    m_roles[ViewCountRole] = "viewCount";
#if QT_VERSION < 0x050000
    setRoleNames(m_roles);
#endif
    m_request->setApiKey(YouTube::instance()->apiKey());
    m_request->setClientId(YouTube::instance()->clientId());
    m_request->setClientSecret(YouTube::instance()->clientSecret());
    m_request->setAccessToken(YouTube::instance()->accessToken());
    m_request->setRefreshToken(YouTube::instance()->refreshToken());
    
    connect(m_request, SIGNAL(accessTokenChanged(QString)), YouTube::instance(), SLOT(setAccessToken(QString)));
    connect(m_request, SIGNAL(refreshTokenChanged(QString)), YouTube::instance(), SLOT(setRefreshToken(QString)));
    connect(m_request, SIGNAL(finished()), this, SLOT(onRequestFinished()));
}

QString YouTubeVideoModel::errorString() const {
    return YouTube::getErrorString(m_request->result().toMap());
}

QYouTube::ResourcesRequest::Status YouTubeVideoModel::status() const {
    if ((m_contentRequest) && (m_contentRequest->status() == QYouTube::ResourcesRequest::Loading)) {
        return QYouTube::ResourcesRequest::Loading;
    }
    
    return m_request->status();
}

#if QT_VERSION >=0x050000
QHash<int, QByteArray> YouTubeVideoModel::roleNames() const {
    return m_roles;
}
#endif

int YouTubeVideoModel::rowCount(const QModelIndex &) const {
    return m_items.size();
}

bool YouTubeVideoModel::canFetchMore(const QModelIndex &) const {
    return (status() != QYouTube::ResourcesRequest::Loading) && (!m_nextPageToken.isEmpty());
}

void YouTubeVideoModel::fetchMore(const QModelIndex &) {
    if (!canFetchMore()) {
        return;
    }
    
    QVariantMap params = m_params;
    params["pageToken"] = m_nextPageToken;
    
    m_request->list(m_resourcePath, m_part, m_filters, params);
    emit statusChanged(status());
}

QVariant YouTubeVideoModel::data(const QModelIndex &index, int role) const {
    if (YouTubeVideo *video = get(index.row())) {
        return video->property(m_roles[role]);
    }
    
    return QVariant();
}

QMap<int, QVariant> YouTubeVideoModel::itemData(const QModelIndex &index) const {
    QMap<int, QVariant> map;
    
    if (YouTubeVideo *video = get(index.row())) {
        QHashIterator<int, QByteArray> iterator(m_roles);
        
        while (iterator.hasNext()) {
            iterator.next();
            map[iterator.key()] = video->property(iterator.value());
        }
    }
    
    return map;
}

QVariant YouTubeVideoModel::data(int row, const QByteArray &role) const {
    if (YouTubeVideo *video = get(row)) {
        return video->property(role);
    }
    
    return QVariant();
}

QVariantMap YouTubeVideoModel::itemData(int row) const {
    QVariantMap map;
    
    if (YouTubeVideo *video = get(row)) {
        foreach (QByteArray role, m_roles.values()) {
            map[role] = video->property(role);
        }
    }
    
    return map;
}

YouTubeVideo* YouTubeVideoModel::get(int row) const {
    if ((row >= 0) && (row < m_items.size())) {
        return m_items.at(row);
    }
    
    return 0;
}

void YouTubeVideoModel::list(const QString &resourcePath, const QStringList &part, const QVariantMap &filters,
                             const QVariantMap &params) {

    if (status() == QYouTube::ResourcesRequest::Loading) {
        return;
    }
    
    clear();
    m_resourcePath = resourcePath;
    m_part = part;
    m_filters = filters;
    m_params = params;
    m_request->list(resourcePath, part, filters, params);
    emit statusChanged(status());
    
    disconnect(YouTube::instance(), 0, this, 0);
        
    if (resourcePath.endsWith("playlistItems")) {
        if (filters.value("playlistId") == YouTube::instance()->relatedPlaylist("favorites")) {
            connect(YouTube::instance(), SIGNAL(videoFavourited(YouTubeVideo*)),
                    this, SLOT(onVideoFavourited(YouTubeVideo*)));
            connect(YouTube::instance(), SIGNAL(videoUnfavourited(YouTubeVideo*)),
                    this, SLOT(onVideoUnfavourited(YouTubeVideo*)));
        }
        else if (filters.value("playlistId") == YouTube::instance()->relatedPlaylist("watchLater")) {
            connect(YouTube::instance(), SIGNAL(videoWatchLater(YouTubeVideo*)),
                    this, SLOT(onVideoWatchLater(YouTubeVideo*)));
        }
        else {
            connect(YouTube::instance(), SIGNAL(videoAddedToPlaylist(YouTubeVideo*, YouTubePlaylist*)),
                    this, SLOT(onVideoAddedToPlaylist(YouTubeVideo*, YouTubePlaylist*)));
            connect(YouTube::instance(), SIGNAL(videoRemovedFromPlaylist(YouTubeVideo*, YouTubePlaylist*)),
                    this, SLOT(onVideoRemovedFromPlaylist(YouTubeVideo*, YouTubePlaylist*)));
        }
    }
}

void YouTubeVideoModel::clear() {
    if (!m_items.isEmpty()) {
        beginResetModel();
        qDeleteAll(m_items);
        m_items.clear();
        m_nextPageToken = QString();
        endResetModel();
        emit countChanged(rowCount());
    }
}

void YouTubeVideoModel::cancel() {
    m_request->cancel();
}

void YouTubeVideoModel::reload() {
    clear();
    m_request->list(m_resourcePath, m_part, m_filters, m_params);
    emit statusChanged(status());
}

void YouTubeVideoModel::getAdditionalContent() {
    if (!m_contentRequest) {
        m_contentRequest = new QYouTube::ResourcesRequest(this);
        m_contentRequest->setApiKey(YouTube::instance()->apiKey());
        m_contentRequest->setClientId(YouTube::instance()->clientId());
        m_contentRequest->setClientSecret(YouTube::instance()->clientSecret());
        m_contentRequest->setAccessToken(YouTube::instance()->accessToken());
        m_contentRequest->setRefreshToken(YouTube::instance()->refreshToken());

        connect(m_contentRequest, SIGNAL(accessTokenChanged(QString)),
                YouTube::instance(), SLOT(setAccessToken(QString)));
        connect(m_contentRequest, SIGNAL(refreshTokenChanged(QString)),
                YouTube::instance(), SLOT(setRefreshToken(QString)));
        connect(m_contentRequest, SIGNAL(finished()), this, SLOT(onContentRequestFinished()));
    }
    
    QStringList ids;
    
    foreach (QVariant result, m_results) {
        ids << YouTube::getVideoId(result.toMap());
    }
    
    QVariantMap filters;
    filters["id"] = ids.join(",");
    
    m_contentRequest->list("/videos", QStringList() << "contentDetails" << "statistics", filters);
}

void YouTubeVideoModel::loadResults() {
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + m_results.size() - 1);
    
    foreach (QVariant result, m_results) {
        m_items << new YouTubeVideo(result.toMap(), this);
    }

    endInsertRows();
    emit countChanged(rowCount());
    emit statusChanged(status());
}

void YouTubeVideoModel::append(YouTubeVideo *video) {
    beginInsertRows(QModelIndex(), m_items.size(), m_items.size());
    m_items << video;
    endInsertRows();
}

void YouTubeVideoModel::insert(int row, YouTubeVideo *video) {
    if ((row >= 0) && (row < m_items.size())) {
        beginInsertRows(QModelIndex(), row, row);
        m_items.insert(row, video);
        endInsertRows();
    }
    else {
        append(video);
    }
}

void YouTubeVideoModel::remove(int row) {
    if ((row >= 0) && (row < m_items.size())) {
        beginRemoveRows(QModelIndex(), row, row);
        m_items.takeAt(row)->deleteLater();
        endRemoveRows();
    }
}

void YouTubeVideoModel::onRequestFinished() {
    if (m_request->status() == QYouTube::ResourcesRequest::Ready) {
        QVariantMap result = m_request->result().toMap();
        
        if (!result.isEmpty()) {
            m_nextPageToken = result.value("nextPageToken").toString();
            m_results = result.value("items").toList();

            if (!m_results.isEmpty()) {
                if (result.value("kind") != "youtube#videoListResponse") {
                    if (result.value("kind") == "youtube#activityListResponse") {
                        foreach (QVariant item, m_results) {
                            if (item.toMap().value("snippet").toMap().value("type") != "upload") {
                                m_results.removeOne(item);
                            }
                        }
                    }
                    
                    getAdditionalContent();
                }
                else {
                    loadResults();
                }

                return;
            }
        }
    }

    emit statusChanged(status());
}

void YouTubeVideoModel::onContentRequestFinished() {
    if (m_contentRequest->status() == QYouTube::ResourcesRequest::Ready) {
        QVariantMap result = m_contentRequest->result().toMap();
        
        if (!result.isEmpty()) {
            QVariantList list = result.value("items").toList();
            const int count = qMin(list.size(), m_results.size());
            
            for (int i = 0; i < count; i++) {
                QVariantMap item = list.at(i).toMap();
                QVariantMap video = m_results.takeAt(i).toMap();
                video["contentDetails"] = item.value("contentDetails");
                video["statistics"] = item.value("statistics");
                m_results.insert(i, video);
            }
        }
    }

    loadResults();
}

void YouTubeVideoModel::onVideoAddedToPlaylist(YouTubeVideo *video, YouTubePlaylist *playlist) {
    if (m_filters.value("playlistId") == playlist->id()) {
        insert(0, new YouTubeVideo(video, this));
    }
#ifdef CUTETUBE_DEBUG
    qDebug() << "YouTubeVideoModel::onVideoAddedToPlaylist" << video->id() << playlist->id();
#endif
}

void YouTubeVideoModel::onVideoRemovedFromPlaylist(YouTubeVideo *video, YouTubePlaylist *playlist) {
    if (m_filters.value("playlistId") == playlist->id()) {
        QModelIndexList list = match(index(0), IdRole, video->id(), 1, Qt::MatchExactly);
        
        if (!list.isEmpty()) {
            remove(list.first().row());
        }
    }
#ifdef CUTETUBE_DEBUG
    qDebug() << "YouTubeVideoModel::onVideoRemovedFromPlaylist" << video->id() << playlist->id();
#endif
}

void YouTubeVideoModel::onVideoFavourited(YouTubeVideo *video) {
    insert(0, new YouTubeVideo(video, this));
#ifdef CUTETUBE_DEBUG
    qDebug() << "YouTubeVideoModel::onVideoFavourited" << video->id();
#endif
}

void YouTubeVideoModel::onVideoUnfavourited(YouTubeVideo *video) {
    QModelIndexList list = match(index(0), IdRole, video->id(), 1, Qt::MatchExactly);
    
    if (!list.isEmpty()) {
        remove(list.first().row());
    }
#ifdef CUTETUBE_DEBUG
    qDebug() << "YouTubeVideoModel::onVideoUnfavourited" << video->id();
#endif
}

void YouTubeVideoModel::onVideoWatchLater(YouTubeVideo *video) {
    insert(0, new YouTubeVideo(video, this));
#ifdef CUTETUBE_DEBUG
    qDebug() << "YouTubeVideoModel::onVideoWatchLater" << video->id();
#endif
}
