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

#include "playlistdelegate.h"
#include "drawing.h"
#include "imagecache.h"
#include <QPainter>
#include <QApplication>

PlaylistDelegate::PlaylistDelegate(ImageCache *cache, int dateRole, int thumbnailRole, int titleRole, int usernameRole,
                                   int videoCountRole, QObject *parent) :
    QStyledItemDelegate(parent),
    m_cache(cache),
    m_dateRole(dateRole),
    m_thumbnailRole(thumbnailRole),
    m_titleRole(titleRole),
    m_usernameRole(usernameRole),
    m_videoCountRole(videoCountRole),
    m_gridMode(false)
{
}

void PlaylistDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    if ((option.state) & (QStyle::State_Selected)) {
        painter->drawImage(option.rect, QImage("/etc/hildon/theme/images/TouchListBackgroundPressed.png"));
    }
    else if (!gridMode()) {
        painter->drawImage(option.rect, QImage("/etc/hildon/theme/images/TouchListBackgroundNormal.png"));
    }
   
    QUrl imageUrl = index.data(m_thumbnailRole).toUrl();
    QImage image = m_cache->image(imageUrl, QSize(100, 75));
    
    QRect imageRect = option.rect;
    imageRect.setLeft(imageRect.left() + (gridMode() ? 12 : 8));
    imageRect.setTop(imageRect.top() + 8);
    imageRect.setWidth(100);
    imageRect.setHeight(75);
    
    painter->fillRect(imageRect, Qt::black);
    drawCenteredImage(painter, imageRect, image);
    
    const int count = index.data(m_videoCountRole).toInt();
                
    QFont font;
    QRect durationRect = imageRect;
    const int durationHeight = durationRect.height() / 4;
    font.setPixelSize(durationHeight);        
    durationRect.setTop(durationRect.bottom() - (durationHeight));
    
    painter->save();
    painter->setOpacity(0.8);
    painter->fillRect(durationRect, Qt::black);
    painter->setFont(font);
    painter->setOpacity(1);
    painter->setPen(Qt::white);
    painter->drawText(durationRect, Qt::AlignCenter, count > 0 ? tr("%1 videos").arg(count) : tr("No videos"));
    painter->restore();
    
    painter->save();
    painter->setPen(QApplication::palette().color(QPalette::Mid));
    painter->drawRect(imageRect);
    painter->setPen(QApplication::palette().color(QPalette::Text));
    
    QRect textRect = option.rect;
    textRect.setLeft(textRect.left() + (gridMode() ? 8 : 116));
    textRect.setTop(gridMode() ? imageRect.bottom() + 8 : textRect.top() + 8);
    textRect.setRight(textRect.right() - 8);
    textRect.setBottom(textRect.bottom() - 8);
    
    QString title = index.data(m_titleRole).toString();
    
    if (gridMode()) {
        font.setPixelSize(16);
        painter->setFont(font);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::TextWordWrap, title);
    }
    else {
        if (textRect.width() < 600) {
            font.setPixelSize(18);
            painter->setFont(font);
        }
        
        QFontMetrics fm = painter->fontMetrics();
        
        title = fm.elidedText(title, Qt::ElideRight, textRect.width());
        QString subTitle;
        QString username = index.data(m_usernameRole).toString();
        QString date = index.data(m_dateRole).toString();
        
        if (!username.isEmpty()) {
            subTitle.append(QString("%1 %2").arg(tr("by")).arg(username));
        }
        
        if (!date.isEmpty()) {
            if (!username.isEmpty()) {
                subTitle.append(QString(" %1 ").arg(tr("on")));
            }
            
            subTitle.append(date);
        }
        
        if (!subTitle.isEmpty()) {
            title.append("\n");
            subTitle = fm.elidedText(subTitle, Qt::ElideRight, textRect.width());
            painter->drawText(textRect, Qt::AlignVCenter, title + subTitle);
        }
        else {                
            painter->drawText(textRect, Qt::AlignVCenter | Qt::TextWordWrap, title);
        }
    }
    
    painter->restore();
}

QSize PlaylistDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const {
    return gridMode() ? QSize(124, 132) : QSize(option.rect.width(), 91);
}

bool PlaylistDelegate::gridMode() const {
    return m_gridMode;
}

void PlaylistDelegate::setGridMode(bool enabled) {
    m_gridMode = enabled;
}
