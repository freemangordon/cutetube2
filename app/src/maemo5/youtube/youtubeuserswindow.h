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

#ifndef YOUTUBEUSERSWINDOW_H
#define YOUTUBEUSERSWINDOW_H

#include "stackedwindow.h"
#include "youtubeusermodel.h"

class UserDelegate;
class ImageCache;
class ListView;
class QLabel;
class QVBoxLayout;
class QActionGroup;

class YouTubeUsersWindow : public StackedWindow
{
    Q_OBJECT
    
public:
    explicit YouTubeUsersWindow(StackedWindow *parent = 0);
    ~YouTubeUsersWindow();
    
public Q_SLOTS:
    void list(const QString &resourcePath, const QStringList &part, const QVariantMap &filters = QVariantMap(),
              const QVariantMap &params = QVariantMap());
    
private Q_SLOTS:
    void enableGridMode();
    void enableListMode();
    
    void showUser(const QModelIndex &index);
        
    void onImageReady();
    void onModelStatusChanged(QYouTube::ResourcesRequest::Status status);
    
private:
    YouTubeUserModel *m_model;
    ImageCache *m_cache;
    
    ListView *m_view;
    UserDelegate *m_delegate;
    QActionGroup *m_viewGroup;
    QAction *m_listAction;
    QAction *m_gridAction;
    QAction *m_reloadAction;
    QLabel *m_label;
    QVBoxLayout *m_layout;
};
    
#endif // YOUTUBEUSERSWINDOW_H
