/***************************************************************************
 *   Copyright (C) 2021 by Stefan Kebekus                                  *
 *   stefan.kebekus@gmail.com                                              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#pragma once

#include <QObject>

namespace Platform {

/*! \brief This class shows platform-native notifications to the user.
 *
 *  The enum NotificationType names a number pre-defined notifications that can be
 *  shown to the user with the method showNotification(). The method hideNotification()
 *  removes a notification. The signal notificationClicked() is emitted when the user
 *  clicks on a notification.
 */

class NotificationManager : public QObject
{
    Q_OBJECT

public:
    /*! \brief Standard constructor
     *
     * @param parent Standard QObject parent pointer
    */
    explicit NotificationManager(QObject* parent = nullptr);

    ~NotificationManager();

    /*! \brief Notification types
     *
     *  This enum lists a number of predefined notification types
     *  only these notifications can be shown.
     */
    enum NotificationType
    {
        DownloadInfo = 0,                 /*< Info that  download is in progress */
        TrafficReceiverSelfTestError = 1, /*< Traffic receiver reports problem on self-test */
        TrafficReceiverRuntimeError = 2   /*< Traffic receiver reports problem while running */
    };
    Q_ENUM(NotificationType)

public slots:
    // Emits the signal "notificationClicked".
    void emitNotificationClicked(Platform::NotificationManager::NotificationType notificationType) {
        emit notificationClicked(notificationType);
    }

    /*! \brief Hides a notification
     *
     *  This method hides a notification that is currently shown.  If the notification is not
     *  shown, this method does nothing.
     *
     *  @param notificationType Type of the notification
     */
    static void hideNotification(Platform::NotificationManager::NotificationType notificationType);

    /*! \brief Shows a notification
     *
     *  This method shows a notification to the user.
     *
     *  @param notificationType Type of the notification
     *
     *  @param title One-line notification title ("Traffic receiver problem")
     *
     *  @param text One-line notification text ("Device INOP · Maintenance required · Battery low")
     *
     *  @param longText If not empty, then the notification might be expandable. When expanded, the one-line "text" is replaced by the content of this "longText".
     *  Depending on the platform, this parameter might also be ignored.
     */
    void showNotification(Platform::NotificationManager::NotificationType notificationType, const QString& title, const QString& text, const QString& longText);

signals:
    /*! \brief Emitted when the user clicks on a notification
     */
    void notificationClicked(Platform::NotificationManager::NotificationType notificationType);

private:
    Q_DISABLE_COPY_MOVE(NotificationManager)
};

}
