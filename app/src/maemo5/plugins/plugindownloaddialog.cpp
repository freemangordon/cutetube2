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

#include "plugindownloaddialog.h"
#include "categorynamemodel.h"
#include "resourcesplugins.h"
#include "settings.h"
#include "transfers.h"
#include "valueselector.h"
#include <QScrollArea>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMessageBox>

PluginDownloadDialog::PluginDownloadDialog(const QString &service, const QString &resourceId, const QUrl &streamUrl,
                                           const QString &title, QWidget *parent) :
    Dialog(parent),
    m_id(resourceId),
    m_url(streamUrl),
    m_title(title),
    m_streamModel(new PluginStreamModel(this)),
    m_subtitleModel(0),
    m_categoryModel(new CategoryNameModel(this)),
    m_scrollArea(new QScrollArea(this)),
    m_subtitleCheckBox(0),
    m_audioCheckBox(new QCheckBox(tr("Convert to audio file"), this)),
    m_streamSelector(new ValueSelector(tr("Video format"), this)),
    m_subtitleSelector(0),
    m_categorySelector(new ValueSelector(tr("Category"), this)),
    m_buttonBox(new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Vertical, this)),
    m_layout(new QHBoxLayout(this))
{
    setWindowTitle(tr("Download video"));
    
    m_streamModel->setService(service);
    
    m_streamSelector->setModel(m_streamModel);
    m_categorySelector->setModel(m_categoryModel);
    m_categorySelector->setValue(Settings::instance()->defaultCategory());
    m_categorySelector->setEnabled(m_categoryModel->rowCount() > 0);
    m_audioCheckBox->setEnabled(QFile::exists("/usr/bin/ffmpeg") || QFile::exists("/usr/bin/avconv"));
        
    QWidget *scrollWidget = new QWidget(m_scrollArea);
    QVBoxLayout *vbox = new QVBoxLayout(scrollWidget);
    vbox->addWidget(m_streamSelector, 0, 0);
    vbox->addWidget(m_audioCheckBox, 1, 0);
    
    if (ResourcesPlugins::instance()->resourceTypeIsSupported(service, Resources::SUBTITLE)) {
        m_subtitleModel = new PluginSubtitleModel(this);
        m_subtitleModel->setService(service);
        m_subtitleCheckBox = new QCheckBox(tr("Download subtitles"), this);
        m_subtitleSelector = new ValueSelector(tr("Subtitles language"), this);
        m_subtitleSelector->setModel(m_subtitleModel);
        m_subtitleSelector->setEnabled(false);
        m_subtitleSelector->setCurrentIndex(qMax(0, m_subtitleModel->match("name",
                                            Settings::instance()->subtitlesLanguage())));
        
        vbox->addWidget(m_subtitleCheckBox, 2, 0);
        vbox->addWidget(m_subtitleSelector, 3, 0);
        vbox->addWidget(m_categorySelector, 4, 0);
        
        connect(m_subtitleModel, SIGNAL(statusChanged(ResourcesRequest::Status)), this,
                SLOT(onSubtitleModelStatusChanged(ResourcesRequest::Status)));
        connect(m_subtitleCheckBox, SIGNAL(toggled(bool)), this, SLOT(onSubtitleCheckBoxToggled(bool)));
        connect(m_subtitleSelector, SIGNAL(valueChanged(QVariant)), this, SLOT(onSubtitlesChanged()));
    }
    else {
        vbox->addWidget(m_categorySelector, 2, 0);
    }
    
    vbox->setContentsMargins(0, 0, 0, 0);
    m_scrollArea->setWidget(scrollWidget);
    m_scrollArea->setWidgetResizable(true);
    
    m_layout->addWidget(m_scrollArea);
    m_layout->addWidget(m_buttonBox, Qt::AlignBottom);
    m_layout->setStretch(0, 1);
    
    connect(m_streamModel, SIGNAL(statusChanged(ResourcesRequest::Status)), this,
            SLOT(onStreamModelStatusChanged(ResourcesRequest::Status)));
    connect(m_categorySelector, SIGNAL(valueChanged(QVariant)), this, SLOT(onCategoryChanged()));
    connect(m_streamSelector, SIGNAL(valueChanged(QVariant)), this, SLOT(onStreamChanged()));
    connect(m_buttonBox, SIGNAL(accepted()), this, SLOT(addDownload()));
    connect(m_buttonBox, SIGNAL(rejected()), this, SLOT(reject()));    
}

void PluginDownloadDialog::showEvent(QShowEvent *e) {
    Dialog::showEvent(e);
    
    if (m_url.isEmpty()) {
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
        m_streamModel->list(m_id);
    }
    else {
        m_streamModel->clear();
        m_streamModel->append(tr("Default format"), m_url);
        m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
    }
}

void PluginDownloadDialog::onCategoryChanged() {
    Settings::instance()->setDefaultCategory(m_categorySelector->valueText());
}

void PluginDownloadDialog::onStreamChanged() {
    Settings::instance()->setDefaultDownloadFormat(m_streamModel->service(), m_streamSelector->valueText());
}

void PluginDownloadDialog::onSubtitlesChanged() {
    Settings::instance()->setSubtitlesLanguage(m_subtitleSelector->valueText());
}

void PluginDownloadDialog::onSubtitleCheckBoxToggled(bool enabled) {
    Settings::instance()->setSubtitlesEnabled(enabled);
    
    if (enabled) {
        if (m_subtitleModel->status() == ResourcesRequest::Null) {
            m_subtitleModel->list(m_id);
        }
    }
    else {
        m_subtitleModel->cancel();
    }
}

void PluginDownloadDialog::onStreamModelStatusChanged(ResourcesRequest::Status status) {
    switch (status) {
    case ResourcesRequest::Loading:
        showProgressIndicator();
        return;
    case ResourcesRequest::Ready:
        if (m_streamModel->rowCount() > 0) {
            m_streamSelector->setCurrentIndex(qMax(0, m_streamModel->match("name",
                                                   Settings::instance()->defaultDownloadFormat(m_streamModel->service()))));
        }
        else {
            QMessageBox::critical(this, tr("Error"), tr("No streams available for '%1'").arg(m_title));
        }
        
        break;
    case ResourcesRequest::Failed:
        QMessageBox::critical(this, tr("Error"), tr("No streams available for '%1'").arg(m_title));
        break;
    default:
        break;
    }
    
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(m_streamModel->rowCount() > 0);
    hideProgressIndicator();
}

void PluginDownloadDialog::onSubtitleModelStatusChanged(ResourcesRequest::Status status) {
    switch (status) {
    case ResourcesRequest::Loading:
        showProgressIndicator();
        return;
    case ResourcesRequest::Ready:
        if (m_subtitleModel->rowCount() > 0) {
            m_subtitleSelector->setCurrentIndex(0);
        }
        
        break;
    case ResourcesRequest::Failed:
        QMessageBox::information(this, tr("Error"), tr("No subtitles available for '%1'").arg(m_title));
        break;
    default:
        break;
    }
    
    m_subtitleCheckBox->setChecked(m_subtitleModel->rowCount() > 0);
    m_subtitleCheckBox->setEnabled(m_subtitleModel->rowCount() > 0);
    hideProgressIndicator();
}

void PluginDownloadDialog::addDownload() {
    QString streamId = m_url.isEmpty() ? m_streamSelector->currentValue().toMap().value("id").toString() : QString();
    QString subtitles = (m_subtitleCheckBox) && (m_subtitleCheckBox->isChecked()) ? m_subtitleSelector->valueText()
                                                                                  : QString();
    QString category = m_categorySelector->valueText();
    Transfers::instance()->addDownloadTransfer(m_streamModel->service(), m_id, streamId, m_url, m_title, category, subtitles,
                                               m_audioCheckBox->isChecked());
    
    accept();
}
