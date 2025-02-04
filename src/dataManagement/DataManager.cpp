/***************************************************************************
 *   Copyright (C) 2019-2021 by Stefan Kebekus                             *
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

#include <QApplication>
#include <QDirIterator>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLockFile>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>

#include <chrono>
#include <utility> 

using namespace std::chrono_literals;

#include "DataManager.h"
#include "Settings.h"


DataManagement::DataManager::DataManager(QObject *parent) :
    GlobalObject(parent),
    _maps_json(QUrl("https://cplx.vm.uni-freiburg.de/storage/enroute-GeoJSONv002/maps.json"),
               QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/maps.json", this)
{
    // Earlier versions of this program constructed files with names ending in ".geojson.geojson"
    // or ".mbtiles.mbtiles". We correct those file names here.
    {
        QStringList offendingFiles;
        QDirIterator fileIterator(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/aviation_maps",
                                  QDir::Files, QDirIterator::Subdirectories);
        while (fileIterator.hasNext()) {
            fileIterator.next();
            if (fileIterator.filePath().endsWith(".geojson.geojson") || fileIterator.filePath().endsWith(".mbtiles.mbtiles")) {
                offendingFiles += fileIterator.filePath();
            }
        }
        foreach(auto offendingFile, offendingFiles)
            QFile::rename(offendingFile, offendingFile.section('.', 0, -2));
    }

    // Construct the Dowloadable object "_maps_json". Let it point to the remote
    // file "maps.json" and wire it up.
    connect(&_maps_json, &DataManagement::Downloadable::downloadingChanged, this, &DataManager::downloadingGeoMapListChanged);
    connect(&_maps_json, &DataManagement::Downloadable::fileContentChanged, this, &DataManager::readGeoMapListFromJSONFile);
    connect(&_maps_json, &DataManagement::Downloadable::fileContentChanged, this, &DataManager::setTimeOfLastUpdateToNow);
    connect(&_maps_json, &DataManagement::Downloadable::error, this, &DataManager::errorReceiver);

    // Cleanup
    connect(qApp, &QApplication::aboutToQuit, this, &DataManagement::DataManager::cleanUp);

    // Wire up the DownloadableGroup _geoMaps
    connect(&_geoMaps, &DataManagement::DownloadableGroup::downloadablesChanged, this, &DataManager::geoMapListChanged);
    connect(&_geoMaps, &DataManagement::DownloadableGroup::filesChanged, this, &DataManager::localFileOfGeoMapChanged);

}


void DataManagement::DataManager::deferredInitialization()
{

    // Wire up the automatic update timer and check if automatic updates are
    // due. The method "autoUpdateGeoMapList" will also set a reasonable timeout
    // value for the timer and start it.
    connect(&_autoUpdateTimer, &QTimer::timeout, this, &DataManager::autoUpdateGeoMapList);
    connect(GlobalObject::settings(), &Settings::acceptedTermsChanged, this, &DataManager::updateGeoMapList);
    if (GlobalObject::settings()->acceptedTerms()) {
        autoUpdateGeoMapList();
    }

    // If there is a downloaded maps.json file, we read it. Otherwise, we start
    // a download.
    if (_maps_json.hasFile()) {
        readGeoMapListFromJSONFile();
    } else {
        if (GlobalObject::settings()->acceptedTerms()) {
            _maps_json.startFileDownload();
        }
    }
}


void DataManagement::DataManager::cleanUp()
{

    // It might be possible for whatever reason that our download directory
    // contains files that we do not know whom they belong to. We hunt down
    // those files and silently delete them.
    foreach(auto path, unattachedFiles()) {
        QFile::remove(path);
    }

    // It might be possible that our download directory contains empty
    // subdirectories. We we remove them all.
    bool didDelete = false;
    do{
        didDelete = false;
        QDirIterator dirIterator(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                                 "/aviation_maps", QDir::Dirs|QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (dirIterator.hasNext()) {
            dirIterator.next();

            QDir dir(dirIterator.filePath());
            if (dir.isEmpty()) {
                dir.removeRecursively();
                didDelete = true;
            }
        }
    }while(didDelete);

    // Clear and delete everything
    foreach(auto geoMapPtr, _geoMaps.downloadables())
        delete geoMapPtr;
}


auto DataManagement::DataManager::describeMapFile(const QString& fileName) -> QString
{
    QFileInfo fi(fileName);
    if (!fi.exists()) {
        return tr("No information available.");
    }
    QString result = QStringLiteral("<table><tr><td><strong>%1 :&nbsp;&nbsp;</strong></td><td>%2</td></tr><tr><td><strong>%3 :&nbsp;&nbsp;</strong></td><td>%4</td></tr></table>")
                         .arg(tr("Installed"),
                              fi.lastModified().toUTC().toString(),
                              tr("File Size"),
                              QLocale::system().formattedDataSize(fi.size(), 1, QLocale::DataSizeSIFormat));

    // Extract infomation from GeoJSON
    if (fileName.endsWith(u".geojson")) {
        QLockFile lockFile(fileName+".lock");
        lockFile.lock();
        QFile file(fileName);
        file.open(QIODevice::ReadOnly);
        auto document = QJsonDocument::fromJson(file.readAll());
        file.close();
        lockFile.unlock();
        QString concatInfoString = document.object()[QStringLiteral("info")].toString();
        if (!concatInfoString.isEmpty()) {
            result += "<p>"+tr("The map data was compiled from the following sources.")+"</p><ul>";
            auto infoStrings = concatInfoString.split(QStringLiteral(";"));
            foreach(auto infoString, infoStrings)
                result += "<li>"+infoString+"</li>";
            result += u"</ul>";
        }
    }

    // Extract infomation from MBTILES
    if (fileName.endsWith(u".mbtiles")) {
        // Open database
        auto databaseConnectionName = "GeoMapProvider::describeMapFile "+fileName;
        auto db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), databaseConnectionName);
        db.setDatabaseName(fileName);
        db.open();
        if (!db.isOpenError()) {
            // Read metadata from database
            QSqlQuery query(db);
            QString intResult;
            if (query.exec(QStringLiteral("select name, value from metadata;"))) {
                while(query.next()) {
                    QString key = query.value(0).toString();
                    if (key == u"json") {
                        continue;
                    }
                    intResult += QStringLiteral("<tr><td><strong>%1 :&nbsp;&nbsp;</strong></td><td>%2</td></tr>")
                                     .arg(key, query.value(1).toString());
                }
            }
            if (!intResult.isEmpty()) {
                result += QStringLiteral("<h4>%1</h4><table>%2</table>").arg(tr("Internal Map Data"), intResult);
            }
            db.close();
        }
    }

    // Extract infomation from text file - this is simply the first line
    if (fileName.endsWith(u".txt")) {
        // Open file and read first line
        QFile dataFile(fileName);
        dataFile.open(QIODevice::ReadOnly);
        auto description = dataFile.readLine();
        result += QString("<p>%1</p>").arg( QString::fromLatin1(description));
    }

    return result;
}


void DataManagement::DataManager::updateGeoMapList()
{
    _maps_json.startFileDownload();
}


void DataManagement::DataManager::errorReceiver(const QString& /*unused*/, QString message)
{
    emit error(std::move(message));
}


void DataManagement::DataManager::localFileOfGeoMapChanged()
{
    // Ok, a local file changed. First, we check if this means that the local
    // file of an unsupported map (=map with invalid URL) is gone. These maps
    // are then no longer wanted. We go through the list and see if we can find
    // any candidates.
    auto geoMaps = _geoMaps.downloadables();
    foreach(auto geoMapPtr, geoMaps) {
        if (geoMapPtr->url().isValid()) {
            continue;
        }
        if (geoMapPtr->hasFile()) {
            continue;
        }

        // Ok, we found an unsupported map without local file. Let's get rid of
        // that.
        _geoMaps.removeFromGroup(geoMapPtr);
        geoMapPtr->deleteLater();
    }
}


void DataManagement::DataManager::readGeoMapListFromJSONFile()
{
    if (!_maps_json.hasFile()) {
        return;
    }

    // List of maps as we have them now
    QVector<QPointer<DataManagement::Downloadable>> oldMaps = _geoMaps.downloadables();

    // To begin, we handle the maps described in the maps.json file. If these
    // maps were already present in the old list, we re-use them. Otherwise, we
    // create new Downloadable objects.
    QJsonParseError parseError{};
    auto doc = QJsonDocument::fromJson(_maps_json.fileContent(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return;
    }

    auto top = doc.object();
    auto baseURL = top.value("url").toString();

    foreach(auto map, top.value("maps").toArray()) {
        auto obj = map.toObject();
        auto mapFileName = obj.value("path").toString();
        auto mapName = mapFileName.section('.',-2,-2);
        auto mapUrlName = baseURL + "/"+ obj.value("path").toString();
        auto fileModificationDateTime = QDateTime::fromString(obj.value("time").toString(), "yyyyMMdd");
        auto fileSize = obj.value("size").toInt();

        // If a map with the given name already exists, update that element, delete its entry in oldMaps
        DataManagement::Downloadable *mapPtr = nullptr;
        foreach(auto geoMapPtr, oldMaps) {
            if (geoMapPtr->objectName() == mapName.section("/", -1 , -1)) {
                mapPtr = geoMapPtr;
                break;
            }
        }

        if (mapPtr != nullptr) {
            // Map exists
            oldMaps.removeAll(mapPtr);
            mapPtr->setRemoteFileDate(fileModificationDateTime);
            mapPtr->setRemoteFileSize(fileSize);
        } else {
            // Construct local file name
            auto localFileName = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/aviation_maps/"+mapFileName;

            // Construct a new downloadable object.
            auto *downloadable = new DataManagement::Downloadable(QUrl(mapUrlName), localFileName, this);
            downloadable->setObjectName(mapName.section("/", -1, -1));
            downloadable->setSection(mapName.section("/", -2, -2));
            downloadable->setRemoteFileDate(fileModificationDateTime);
            downloadable->setRemoteFileSize(fileSize);
            _geoMaps.addToGroup(downloadable);
            if (localFileName.endsWith("geojson")) {
                _aviationMaps.addToGroup(downloadable);
            }
            if (localFileName.endsWith("mbtiles")) {
                _baseMaps.addToGroup(downloadable);
            }
            if (localFileName.endsWith("txt")) {
                _databases.addToGroup(downloadable);
            }
        }

    }

    // Now go through all the leftover objects in the old list of aviation maps.
    // These are now aviation maps that are no longer supported. If they have no
    // local file to them, we simply delete them.  If they have a local file, we
    // keep them, but set their QUrl to invalid; this will mark them as
    // unsupported in the GUI.
    foreach(auto geoMapPtr, oldMaps) {
        if (geoMapPtr->hasFile()) {
            continue;
        }
        delete geoMapPtr;
    }

    // Now it is still possible that the download directory contains files
    // beloning to unsupported maps. Add those to newMaps.
    foreach(auto path, unattachedFiles()) {
        // Generate proper object name from path
        QString objectName = path;
        objectName = objectName.remove(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
                                       "/aviation_maps/").section('.', 0, 0);

        auto *downloadable = new DataManagement::Downloadable(QUrl(), path, this);
        downloadable->setSection("Unsupported Maps");
        downloadable->setObjectName(objectName);
        _geoMaps.addToGroup(downloadable);
    }
}


void DataManagement::DataManager::setTimeOfLastUpdateToNow()
{
    // Save timestamp, so that we know when an automatic update is due
    QSettings settings;
    settings.setValue("DataManager/MapListTimeStamp", QDateTime::currentDateTimeUtc());

    // Now that we downloaded successfully, we need to check for updates only once
    // a day
    _autoUpdateTimer.start(24h);
}


void DataManagement::DataManager::autoUpdateGeoMapList()
{
    // If the last update is more than one day ago, automatically initiate an
    // update, so that maps stay at least roughly current.
    QSettings settings;
    QDateTime lastUpdate = settings.value("DataManager/MapListTimeStamp", QDateTime()).toDateTime();

    if (!lastUpdate.isValid() || (qAbs(lastUpdate.daysTo(QDateTime::currentDateTime()) > 6)) ) {
        // Updates are due. Check again in one hour if the update went well or
        // if we need to try again.
        _autoUpdateTimer.start(1h);
        updateGeoMapList();
        return;
    }

    // Updates are not yet due. Check again in one day.
    _autoUpdateTimer.start(24h);
}


auto DataManagement::DataManager::unattachedFiles() const -> QList<QString>
{
    QList<QString> result;

    // It might be possible for whatever reason that our download directory
    // contains files that we do not know whom they belong to. We hunt down
    // those files.
    QDirIterator fileIterator(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)+"/aviation_maps",
                              QDir::Files, QDirIterator::Subdirectories);
    while (fileIterator.hasNext()) {
        fileIterator.next();

        // Now check if this file exists as the local file of some geographic
        // map
        bool isAttachedToAviationMap = false;
        foreach(auto geoMapPtr, _geoMaps.downloadables()) {
            if (geoMapPtr->fileName() == QFileInfo(fileIterator.filePath()).absoluteFilePath()) {
                isAttachedToAviationMap = true;
                break;
            }
        }
        if (!isAttachedToAviationMap) {
            result.append(fileIterator.filePath());
        }
    }

    return result;
}
