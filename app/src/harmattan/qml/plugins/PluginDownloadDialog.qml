/*
 * Copyright (C) 2015 Stuart Howarth <showarth@marxoft.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU Lesser General Public License,
 * version 3, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
 * more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
 */

import QtQuick 1.1
import com.nokia.meego 1.0
import cuteTube 2.0
import ".."
import "file:///usr/lib/qt4/imports/com/nokia/meego/UIConstants.js" as UI

MySheet {
    id: root

    property string resourceId
    property string resourceTitle
    property string streamUrl

    showProgressIndicator: (streamModel.status == ResourcesRequest.Loading)
                           || (subtitleModel.status == ResourcesRequest.Loading)
    acceptButtonText: streamModel.count > 0 ? qsTr("Done") : ""
    rejectButtonText: qsTr("Cancel")
    content: Item {
        anchors.fill: parent

        Flickable {
            id: flicker

            anchors.fill: parent
            contentHeight: column.height + UI.PADDING_DOUBLE

            Column {
                id: column

                anchors {
                    left: parent.left
                    right: parent.right
                    top: parent.top
                }

                ValueSelector {
                    id: streamSelector

                    width: parent.width
                    title: qsTr("Video format")
                    model: PluginStreamModel {
                        id: streamModel

                        service: Settings.currentService
                        onStatusChanged: {
                            switch (status) {
                            case ResourcesRequest.Loading: {
                                streamSelector.showProgressIndicator = true;
                                return;
                            }
                            case ResourcesRequest.Ready:
                                if (count > 0) {
                                    streamSelector.selectedIndex = Math.max(0, match("name",
                                                                            Settings.defaultDownloadFormat(service)));
                                }
                                else {
                                    infoBanner.showMessage(qsTr("No streams found"));
                                }

                                break;
                            case ResourcesRequest.Failed: {
                                infoBanner.showMessage(errorString);
                                break;
                            }
                            default:
                                break;
                            }

                            streamSelector.showProgressIndicator = false;
                        }
                    }
                    onValueChanged: Settings.setDefaultDownloadFormat(streamModel.service, subTitle)
                }
                
                MySwitch {
                    id: audioSwitch

                    text: qsTr("Convert to audio file")
                    enabled: AUDIO_CONVERTOR_ENABLED
                }

                ValueSelector {
                    id: categorySelector

                    width: parent.width
                    title: qsTr("Category")
                    model: CategoryNameModel {
                        id: categoryModel
                    }
                    value: Settings.defaultCategory
                    onValueChanged: Settings.defaultCategory = value
                }
                
                MySwitch {
                    id: subtitleSwitch
            
                    text: qsTr("Download subtitles")
                    onCheckedChanged: {
                        if (checked) {
                            if (subtitleModel.status != ResourcesRequest.Loading) {
                                subtitleModel.list(root.resourceId);
                            }
                        }
                        else {
                            subtitleModel.cancel();
                        }
                    }
                }
                
                ValueSelector {
                    id: subtitleSelector

                    width: parent.width
                    title: qsTr("Video format")
                    enabled: subtitleSwitch.checked
                    model: PluginSubtitleModel {
                        id: subtitleModel

                        service: Settings.currentService
                        onStatusChanged: {
                            switch (status) {
                            case ResourcesRequest.Loading: {
                                subtitleSelector.showProgressIndicator = true;
                                return;
                            }
                            case ResourcesRequest.Ready:
                                if (count > 0) {
                                    subtitleSelector.selectedIndex = Math.max(0, match("name",
                                                                                       Settings.subtitlesLanguage));
                                }
                                else {
                                    infoBanner.showMessage(qsTr("No subtitles found"));
                                }

                                break;
                            case ResourcesRequest.Failed: {
                                infoBanner.showMessage(errorString);
                                break;
                            }
                            default:
                                break;
                            }

                            subtitleSelector.showProgressIndicator = false;
                        }
                    }
                    onAccepted: Settings.setDefaultDownloadFormat(streamModel.service,
                                                                  streamModel.data(selectedIndex, "name"))
                }
            }
        }

        ScrollDecorator {
            flickableItem: flicker
        }
    }

    onAccepted: Transfers.addDownloadTransfer(streamModel.service, resourceId, streamUrl ? "" : streamSelector.value.id,
                                              streamUrl, resourceTitle, Settings.defaultCategory,
                                              subtitleSwitch.checked ?
                                              subtitleModel.data(subtitleSelector.selectedIndex, "name") : "",
                                              audioSwitch.checked)

    onStatusChanged: {
        switch (status) {
        case DialogStatus.Opening: {
            subtitleModel.clear();
            subtitleSwitch.checked = false;
            subtitleSwitch.enabled = Plugins.resourceTypeIsSupported(subtitleModel.service, Resources.SUBTITLE);
            
            if (streamUrl) {
                streamModel.clear();
                streamModel.append(qsTr("Default format"), streamUrl);
            }
            else {
                streamModel.list(resourceId);
            }
            
            break;
        }
        case DialogStatus.Closing: {
            streamModel.cancel();
            subtitleModel.cancel();
            break;
        }
        default:
            break;
        }
    }
}
