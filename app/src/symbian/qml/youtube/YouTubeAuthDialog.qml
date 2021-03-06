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
import QtWebKit 1.0
import ".."

MySheet {
    id: root

    signal codeReady(string code)

    rejectButtonText: qsTr("Cancel")
    content: Item {
        anchors.fill: parent

        MyFlickable {
            id: flicker

            anchors.fill: parent
            contentWidth: webView.width
            contentHeight: webView.height
            boundsBehavior: Flickable.DragOverBounds
            clip: true
            visible: webView.url != ""

            WebView {
                id: webView

                width: 854
                height: 854
                preferredWidth: root.width
                preferredHeight: height
                settings.privateBrowsingEnabled: true
                opacity: status == WebView.Loading ? 0 : 1
                onTitleChanged: {
                    if (/code=/i.test(title)) {
                        root.codeReady(title.split("code=")[1].split("&")[0]);
                        root.accept();
                    }
                }
            }
        }

        MyScrollBar {
            flickableItem: flicker
        }

        BusyIndicator {
            anchors.centerIn: parent
            width: platformStyle.graphicSizeLarge
            height: platformStyle.graphicSizeLarge
            visible: (webView.status == WebView.Loading) && (root.status === DialogStatus.Open)
            running: visible
        }
    }

    onStatusChanged: {
        switch (status) {
        case DialogStatus.Open: {
            CookieJar.setAllCookies([]);
            webView.url = YouTube.authUrl();
            return;
        }
        default:
            return;
        }
    }
}
