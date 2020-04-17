/***************************************************************************
 *   Copyright (C) 2019-2020 by Stefan Kebekus                             *
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

#ifndef DownloadableGroupWatcher_H
#define DownloadableGroupWatcher_H


#include "Downloadable.h"


/*! \brief Manages a set of downloadable objects
 *
 * This convenience class collects signals and properties from a set of
 * Downloadable objects, and forwards summarized information.
 */

class DownloadableGroupWatcher : public QObject
{
    Q_OBJECT

public:
    /*! \brief Constructs an empty group
     *
     * @param parent The standard QObject parent pointer.
     */
    explicit DownloadableGroupWatcher(QObject *parent=nullptr);

    // No copy constructor
    DownloadableGroupWatcher(DownloadableGroupWatcher const&) = delete;

    // No assign operator
    DownloadableGroupWatcher& operator =(DownloadableGroupWatcher const&) = delete;

    // No move constructor
    DownloadableGroupWatcher(DownloadableGroupWatcher&&) = delete;

    // No move assignment operator
    DownloadableGroupWatcher& operator=(DownloadableGroupWatcher&&) = delete;

    /*! \brief List of Downloadables in this group
     *
     * This property holds the list of downloadables in the group. The
     * downloadables are sorted alphabetically in ascending order, first be
     * section() and then secondly by file name. The nullptr is never contained
     * in the list.
     */
    Q_PROPERTY(QList<QPointer<Downloadable>> downloadables READ downloadables NOTIFY downloadablesChanged)

    /*! \brief Getter function for the property with the same name
     *
     * @returns Property downloadables
     */
    QList<QPointer<Downloadable>> downloadables() const;

    /*! \brief List of Downloadables in this group, as a list of QObjects

      This property is identical to downloadables, but returns the pointers to the
      Downloadables in the form of a QObjectList
    */
    Q_PROPERTY(QList<QObject*> downloadablesAsObjectList READ downloadablesAsObjectList NOTIFY downloadablesChanged)

    /*! \brief Getter function for the property with the same name
     *
     * @returns Property downloadables
     */
    QList<QObject*> downloadablesAsObjectList() const;

    /*! \brief List of Downloadable objects in this group that have local files
     *
     * This property holds the list of Downloadable objects in the group that have local files. The
     * Downloadable objects are sorted alphabetically in ascending order, first be
     * section() and then secondly by file name. The nullptr is never contained
     * in the list.
     */
    Q_PROPERTY(QList<QPointer<Downloadable>> downloadablesWithFile READ downloadablesWithFile NOTIFY downloadablesWithFileChanged)

    /*! \brief Getter function for the property with the same name
     *
     * @returns Property downloadablesWithFiles
     */
    QList<QPointer<Downloadable>> downloadablesWithFile() const;

    /*! \brief Indicates whether a download process is currently running
     *
     * By definition, an empty group is not downloading
     */
    Q_PROPERTY(bool downloading READ downloading NOTIFY downloadingChanged)

    /*! \brief Getter function for the property with the same name
     *
     * @returns Property downloading
     */
    bool downloading() const;

    /*! \brief Names of all files that have been downloaded by any of the
     *  Downloadbles in this group
     */
    Q_PROPERTY(QStringList files READ files NOTIFY filesChanged)

    /*! \brief Getter function for the property with the same name
     *
     * @returns Property files
     */
    QStringList files() const;

    /*! \brief True is one of the Downloadable objects has a local file */
    Q_PROPERTY(bool hasFile READ hasFile NOTIFY hasFileChanged)

    /*! \brief Getter function for the property with the same name
     *
     * @returns Property hasFile
     */
    bool hasFile() const;

    /*! \brief Indicates any one of Downloadable objects is known to be updatable
     *
     * By definition, an empty group is not updatable
     */
    Q_PROPERTY(bool updatable READ updatable NOTIFY updatableChanged)

    /*! \brief Getter function for the property with the same name
     *
     * @returns Property updatable
     */
    bool updatable() const;

    /*! \brief \brief Gives an estimate for the download size for all updates in this group, as a localized string
     *
     * The string returned is typically of the form "23.7 MB"
     */
    Q_PROPERTY(QString updateSize READ updateSize NOTIFY updateSizeChanged)

    /*! \brief Getter function for the property with the same name
     *
     * @returns Property updateSize
     */
    QString updateSize() const;

public slots:
    /*! Update all updatable Downloadable objects */
    void updateAll();

signals:
    /*! \brief Notifier signal for property downloading */
    void downloadablesWithFileChanged(QList<QPointer<Downloadable>>);

    /*! \brief Notifier signal for property downloading */
    void downloadingChanged(bool);

    /*! \brief Notifier signal for the property localFiles */
    void filesChanged(QStringList);

    /*! \brief Notifier signal for the property localFiles */
    void hasFileChanged(bool);

    /*! \brief Notifier signal for the property updatable */
    void updatableChanged(bool);

    /*! \brief Notifier signal for the property updatable */
    void updateSizeChanged(QString);

    /*! \brief Emitted if the content of one of the local files changes.
     *
     * This signal is emitted if one of the downloadables in this group emits
     * the signal localFileContentChanged().
     */
    void localFileContentChanged();

    /*! \brief Notifier signal for the property downloadables */
    void downloadablesChanged();

protected slots:
    // This slot is called whenever a Downloadable in this group changes.
    // It compares if one of the _cached… values has changed and emits
    // the appropriate notification signals.
    void checkAndEmitSignals();

    // Remove all instances of nullptr from _downloadables
    void cleanUp();

protected:
    bool                          _cachedDownloading {false};        // Cached value for the 'downloading' property
    QList<QPointer<Downloadable>> _cachedDownloadablesWithFile {};   // Cached value for the 'downloadablesWithFiles' property
    QStringList                   _cachedFiles {};                   // Cached value for the 'files' property
    bool                          _cachedHasFile {false};            // Cached value for the 'hasLocalFile' property
    bool                          _cachedUpdatable {false};          // Cached value for the 'updatable' property
    QString                       _cachedUpdateSize {};              // Cached value for the 'updateSize' property

    // List of QPointers to the Downloadable objects in this group
    QList<QPointer<Downloadable>> _downloadables;
};

#endif // DownloadableGroupWatcher_H
