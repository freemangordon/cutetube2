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

#include "vimeoplaybackdialog.h"
#include "resources.h"
#include "settings.h"
#include "valueselector.h"
#include "videolauncher.h"
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>

VimeoPlaybackDialog::VimeoPlaybackDialog(const QString &resourceId, const QString &title, QWidget *parent) :
    Dialog(parent),
    m_id(resourceId),
    m_title(title),
    m_model(new VimeoStreamModel(this)),
    m_streamSelector(new ValueSelector(tr("Video format"), this)),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Vertical, this)),
    m_layout(new QHBoxLayout(this))
{
    setWindowTitle(tr("Play video"));
    
    m_streamSelector->setModel(m_model);
    
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    
    m_layout->addWidget(m_streamSelector, Qt::AlignBottom);
    m_layout->addWidget(m_buttonBox, Qt::AlignBottom);
    m_layout->setStretch(0, 1);
    
    connect(m_model, SIGNAL(statusChanged(QVimeo::StreamsRequest::Status)), this,
            SLOT(onModelStatusChanged(QVimeo::StreamsRequest::Status)));
    connect(m_streamSelector, SIGNAL(valueChanged(QVariant)), this, SLOT(onStreamChanged()));
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(playVideo()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void VimeoPlaybackDialog::showEvent(QShowEvent *e) {
    Dialog::showEvent(e);
    m_model->list(m_id);
}

void VimeoPlaybackDialog::onModelStatusChanged(QVimeo::StreamsRequest::Status status) {
    switch (status) {
    case QVimeo::StreamsRequest::Loading:
        showProgressIndicator();
        return;
    case QVimeo::StreamsRequest::Ready:
        if (m_model->rowCount() > 0) {
            m_streamSelector->setCurrentIndex(qMax(0, m_model->match("name",
                                                   Settings::instance()->defaultPlaybackFormat(Resources::VIMEO))));
        }
        else {
            QMessageBox::critical(this, tr("Error"), tr("No streams available for '%1'").arg(m_title));
        }
        
        break;
    case QVimeo::StreamsRequest::Failed:
        QMessageBox::critical(this, tr("Error"), tr("No streams available for '%1'").arg(m_title));
        break;
    default:
        break;
    }
    
    hideProgressIndicator();
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_model->rowCount() > 0);
}

void VimeoPlaybackDialog::onStreamChanged() {
    Settings::instance()->setDefaultPlaybackFormat(Resources::VIMEO, m_streamSelector->valueText());
}

void VimeoPlaybackDialog::playVideo() {
    VideoLauncher::playVideo(m_streamSelector->currentValue().toMap().value("url").toString());
    accept();
}
