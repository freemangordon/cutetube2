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
import com.nokia.symbian 1.1
import cuteTube 2.0
import QVimeo 1.0 as QVimeo
import ".."

Tab {
    id: root

    property alias model: videoModel

    title: qsTr("Videos")
    showProgressIndicator: (video.status == QVimeo.ResourcesRequest.Loading)
                           || (videoModel.status == QVimeo.ResourcesRequest.Loading)
    tools: ToolBarLayout {

        BackToolButton {}

        MyToolButton {
            iconSource: "toolbar-refresh"
            toolTipText: qsTr("Reload")
            enabled: videoModel.status != QVimeo.ResourcesRequest.Loading
            onClicked: videoModel.reload()
        }
    }

    VimeoVideo {
        id: video

        onStatusChanged: if (status == QVimeo.ResourcesRequest.Failed) infoBanner.showMessage(errorString);
    }

    MyListView {
        id: view

        anchors.fill: parent
        cacheBuffer: 400
        model: VimeoVideoModel {
            id: videoModel

            onStatusChanged: if (status == QVimeo.ResourcesRequest.Failed) infoBanner.showMessage(errorString);
        }
        delegate: VideoDelegate {
            onClicked: appWindow.pageStack.push(Qt.resolvedUrl("VimeoVideoPage.qml")).load(videoModel.get(index))
            onThumbnailClicked: {
                if (Settings.videoPlayer == "cutetube") {
                    appWindow.pageStack.push(Qt.resolvedUrl("../VideoPlaybackPage.qml")).addVideos([videoModel.get(index)]);
                }
                else {
                    dialogLoader.sourceComponent = playbackDialog;
                    dialogLoader.item.model.list(id);
                    dialogLoader.item.open();
                }
            }

            onPressAndHold: {
                view.currentIndex = -1;
                view.currentIndex = index;
                contextMenu.open();
            }
        }
    }

    MyScrollBar {
        flickableItem: view
    }

    Label {
        anchors {
            fill: parent
            margins: platformStyle.paddingLarge
        }
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        wrapMode: Text.WordWrap
        font.pixelSize: 40
        color: platformStyle.colorNormalMid
        text: qsTr("No videos found")
        visible: (videoModel.status >= QVimeo.ResourcesRequest.Ready) && (videoModel.count == 0)
    }

    MyContextMenu {
        id: contextMenu

        focusItem: view

        MenuLayout {

            MenuItem {
                text: qsTr("Download")
                onClicked: {
                    dialogLoader.sourceComponent = downloadDialog;
                    dialogLoader.item.resourceId = videoModel.data(view.currentIndex, "id");
                    dialogLoader.item.resourceTitle = videoModel.data(view.currentIndex, "title");
                    dialogLoader.item.open();
                }
            }

            MenuItem {
                text: qsTr("Copy URL")
                onClicked: {
                    Clipboard.text = videoModel.data(view.currentIndex, "url");
                    infoBanner.showMessage(qsTr("URL copied to clipboard"));
                }
            }

            MenuItem {
                text: videoModel.data(view.currentIndex, "favourited") ? qsTr("Unlike") : qsTr("Like")
                enabled: (Vimeo.userId) && (Vimeo.hasScope(Vimeo.INTERACT_SCOPE))
                onClicked: {
                    video.loadVideo(videoModel.get(view.currentIndex))

                    if (video.favourited) {
                        video.unfavourite();
                    }
                    else {
                        video.favourite();
                    }
                }
            }

            MenuItem {
                text: qsTr("Watch later")
                enabled: (Vimeo.userId) && (Vimeo.hasScope(Vimeo.INTERACT_SCOPE))
                onClicked: {
                    video.loadVideo(videoModel.get(view.currentIndex))
                    video.watchLater();
                }
            }

            MenuItem {
                text: qsTr("Add to album")
                enabled: (Vimeo.userId) && (Vimeo.hasScope(Vimeo.EDIT_SCOPE))
                onClicked: {
                    dialogLoader.sourceComponent = playlistDialog;
                    dialogLoader.item.load(videoModel.get(view.currentIndex));
                    dialogLoader.item.open();
                }
            }
        }
    }

    Loader {
        id: dialogLoader
    }

    Component {
        id: playbackDialog

        VimeoPlaybackDialog {
            focusItem: view
            onAccepted: VideoLauncher.playVideo(value.url)
        }
    }

    Component {
        id: downloadDialog

        VimeoDownloadDialog {
            focusItem: view
        }
    }

    Component {
        id: playlistDialog

        VimeoPlaylistDialog {
            focusItem: view
        }
    }
}
