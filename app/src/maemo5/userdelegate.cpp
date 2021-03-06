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

#include "userdelegate.h"
#include "drawing.h"
#include "imagecache.h"
#include "utils.h"
#include <QPainter>
#include <QApplication>

UserDelegate::UserDelegate(ImageCache *cache, int subscriberCountRole, int thumbnailRole, int usernameRole,
                           QObject *parent) :
    QStyledItemDelegate(parent),
    m_cache(cache),
    m_subscriberCountRole(subscriberCountRole),
    m_thumbnailRole(thumbnailRole),
    m_usernameRole(usernameRole),
    m_gridMode(false)
{
}

void UserDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    painter->save();
    
    if ((option.state) & (QStyle::State_Selected)) {
        painter->drawImage(option.rect, QImage("/etc/hildon/theme/images/TouchListBackgroundPressed.png"));
    }
    else if (!gridMode()) {
        painter->drawImage(option.rect, QImage("/etc/hildon/theme/images/TouchListBackgroundNormal.png"));
    }
    
    QUrl imageUrl = index.data(m_thumbnailRole).toUrl();
    QImage image = m_cache->image(imageUrl, QSize(75, 75));
    
    QRect imageRect = option.rect;
    imageRect.setLeft(imageRect.left() + (gridMode() ? 9 : 8));
    imageRect.setTop(imageRect.top() + 8);
    imageRect.setWidth(75);
    imageRect.setHeight(75);
    
    drawCenteredImage(painter, imageRect, image);
        
    QRect textRect = option.rect;
    textRect.setLeft(textRect.left() + (gridMode() ? 8 : 91));
    textRect.setTop(gridMode() ? imageRect.bottom() + 8 : textRect.top() + 8);
    textRect.setRight(textRect.right() - 8);
    textRect.setBottom(textRect.bottom() - 8);
    
    QString text = index.data(m_usernameRole).toString();
    
    if (gridMode()) {
        QFont font;
        font.setPixelSize(16);
        painter->setFont(font);
        painter->drawText(textRect, Qt::AlignVCenter | Qt::TextWordWrap, text);
    }
    else {
        if (textRect.width() < 600) {
            QFont font;
            font.setPixelSize(18);
            painter->setFont(font);
        }
        
        text = painter->fontMetrics().elidedText(text, Qt::ElideRight, textRect.width());
        
        const qint64 count = index.data(m_subscriberCountRole).toLongLong();
        text.append("\n" + (count > 0 ? tr("%1 subscribers").arg(Utils::formatLargeNumber(count)) : tr("No subscribers")));
                
        painter->drawText(textRect, Qt::AlignVCenter, text);
    }
    
    painter->restore();
}

QSize UserDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &) const {
    return gridMode() ? QSize(93, 116) : QSize(option.rect.width(), 91);
}

bool UserDelegate::gridMode() const {
    return m_gridMode;
}

void UserDelegate::setGridMode(bool enabled) {
    m_gridMode = enabled;
}
