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

#ifndef PLUGINVIDEOSWINDOW_H
#define PLUGINVIDEOSWINDOW_H

#include "stackedwindow.h"
#include "pluginvideomodel.h"

class VideoDelegate;
class ImageCache;
class ListView;
class QLabel;
class QVBoxLayout;
class QActionGroup;
class QMenu;

class PluginVideosWindow : public StackedWindow
{
    Q_OBJECT
    
public:
    explicit PluginVideosWindow(StackedWindow *parent = 0);
    ~PluginVideosWindow();

public Q_SLOTS:
    void list(const QString &service, const QString &id = QString());
    void search(const QString &service, const QString &query, const QString &order);
    
private Q_SLOTS:
    void enableGridMode();
    void enableListMode();
    
    void downloadVideo();
    void playVideo(const QModelIndex &index);
    void shareVideo();
    void showVideo(const QModelIndex &index);
    
    void showContextMenu(const QPoint &pos);
    
    void onImageReady();
    void onModelStatusChanged(ResourcesRequest::Status status);
    
private:
    PluginVideoModel *m_model;
    ImageCache *m_cache;
    
    ListView *m_view;
    VideoDelegate *m_delegate;
    QActionGroup *m_viewGroup;
    QAction *m_listAction;
    QAction *m_gridAction;
    QAction *m_reloadAction;
    QMenu *m_contextMenu;
    QAction *m_downloadAction;
    QAction *m_shareAction;
    QLabel *m_label;
    QVBoxLayout *m_layout;
};
    
#endif // PLUGINVIDEOSWINDOW_H
