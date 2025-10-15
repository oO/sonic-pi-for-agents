//--
// This file is part of Sonic Pi: http://sonic-pi.net
// Full project source: https://github.com/samaaron/sonic-pi
// License: https://github.com/samaaron/sonic-pi/blob/main/LICENSE.md
//
// Copyright 2024 Sonic Pi for Agents
// All rights reserved.
//
// Permission is granted for use, copying, modification, and
// distribution of modified versions of this work as long as this
// notice is included.
//++

#include "sonicpiproject.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>

// Public default constructor (required by Qt meta-object system)
SonicPiProject::SonicPiProject(QObject* parent)
    : QObject(parent)
    , m_projectPath(QString())
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_pollTimer(new QTimer(this))
{
    // Don't initialize file watcher for empty projects
}

// Private constructor used by create() and load() factory methods
SonicPiProject::SonicPiProject(const QString& projectPath, QObject* parent)
    : QObject(parent)
    , m_projectPath(projectPath)
    , m_fileWatcher(new QFileSystemWatcher(this))
    , m_pollTimer(new QTimer(this))
{
    initFileWatcher();
}

SonicPiProject::~SonicPiProject()
{
    m_pollTimer->stop();
}

SonicPiProject* SonicPiProject::create(const QString& folderPath, QObject* parent)
{
    QDir dir(folderPath);

    // Create directory if it doesn't exist
    if (!dir.exists())
    {
        if (!dir.mkpath("."))
        {
            qWarning() << "Failed to create project directory:" << folderPath;
            return nullptr;
        }
    }

    // Create 10 buffer files with default content
    for (int i = 0; i < 10; i++)
    {
        QString filename = QString("buffer.%1.spi").arg(i);
        QString filePath = dir.filePath(filename);

        // Only create if doesn't exist
        if (!QFile::exists(filePath))
        {
            QFile file(filePath);
            if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                qWarning() << "Failed to create buffer file:" << filePath;
                return nullptr;
            }

            QTextStream out(&file);
            out << "# Buffer " << i << "\n";
            out << "# Welcome to Sonic Pi for Agents\n\n";
            file.close();
        }
    }

    qDebug() << "Created project at:" << folderPath;
    return new SonicPiProject(folderPath, parent);
}

SonicPiProject* SonicPiProject::load(const QString& folderPath, QObject* parent)
{
    QDir dir(folderPath);

    if (!dir.exists())
    {
        qWarning() << "Project folder does not exist:" << folderPath;
        return nullptr;
    }

    // Check that all 10 buffer files exist
    for (int i = 0; i < 10; i++)
    {
        QString filename = QString("buffer.%1.spi").arg(i);
        QString filePath = dir.filePath(filename);

        if (!QFile::exists(filePath))
        {
            qWarning() << "Missing buffer file:" << filePath;
            // Create missing files
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text))
            {
                QTextStream out(&file);
                out << "# Buffer " << i << "\n\n";
                file.close();
            }
        }
    }

    qDebug() << "Loaded project from:" << folderPath;
    return new SonicPiProject(folderPath, parent);
}

QString SonicPiProject::bufferFileName(int bufferId) const
{
    return QString("buffer.%1.spi").arg(bufferId);
}

QString SonicPiProject::getBufferFilePath(int bufferId) const
{
    if (bufferId < 0 || bufferId >= 10)
    {
        qWarning() << "Invalid buffer ID:" << bufferId;
        return QString();
    }

    return QDir(m_projectPath).filePath(bufferFileName(bufferId));
}

QString SonicPiProject::readBuffer(int bufferId) const
{
    QString filePath = getBufferFilePath(bufferId);
    if (filePath.isEmpty())
        return QString();

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to read buffer file:" << filePath;
        return QString();
    }

    QTextStream in(&file);
    QString content = in.readAll();
    file.close();

    return content;
}

bool SonicPiProject::writeBuffer(int bufferId, const QString& content)
{
    QString filePath = getBufferFilePath(bufferId);
    if (filePath.isEmpty())
        return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qWarning() << "Failed to write buffer file:" << filePath;
        return false;
    }

    QTextStream out(&file);
    out << content;
    file.close();

    // Track this write to avoid false external change detection
    m_lastLocalWrite[bufferId] = QDateTime::currentDateTime();
    m_lastKnownContent[bufferId] = content;

    qDebug() << "Wrote buffer" << bufferId << "to" << filePath;
    return true;
}

bool SonicPiProject::isValid() const
{
    QDir dir(m_projectPath);
    if (!dir.exists())
        return false;

    // Check all 10 buffer files exist
    for (int i = 0; i < 10; i++)
    {
        if (!QFile::exists(getBufferFilePath(i)))
            return false;
    }

    return true;
}

void SonicPiProject::initFileWatcher()
{
    // Watch all buffer files
    for (int i = 0; i < 10; i++)
    {
        QString filePath = getBufferFilePath(i);
        m_fileWatcher->addPath(filePath);

        // Initialize last known state
        m_lastKnownContent[i] = readBuffer(i);
        QFileInfo info(filePath);
        m_lastModified[i] = info.lastModified();
    }

    // Connect file watcher signal
    connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
            this, &SonicPiProject::onFileChanged);

    // Set up polling timer (100ms for responsive updates)
    m_pollTimer->setInterval(100);
    connect(m_pollTimer, &QTimer::timeout,
            this, &SonicPiProject::onPollTimeout);
    m_pollTimer->start();

    qDebug() << "File watcher initialized for project:" << m_projectPath;
}

void SonicPiProject::onFileChanged(const QString& path)
{
    // QFileSystemWatcher can be unreliable, so we also poll
    // This is just an early notification
    checkForExternalChanges();
}

void SonicPiProject::onPollTimeout()
{
    checkForExternalChanges();
}

void SonicPiProject::checkForExternalChanges()
{
    for (int i = 0; i < 10; i++)
    {
        QString filePath = getBufferFilePath(i);
        QFileInfo info(filePath);

        if (!info.exists())
            continue;

        QDateTime modified = info.lastModified();

        // Check if file was modified since we last checked
        if (modified > m_lastModified[i])
        {
            // Check if this was our own write (within 200ms window)
            QDateTime lastWrite = m_lastLocalWrite.value(i);
            qint64 msSinceWrite = lastWrite.msecsTo(modified);

            if (msSinceWrite < 200 && !lastWrite.isNull())
            {
                // This was likely our own write, ignore
                m_lastModified[i] = modified;
                continue;
            }

            // This is an external change
            QString newContent = readBuffer(i);

            // Check if content actually changed
            if (newContent != m_lastKnownContent[i])
            {
                m_lastKnownContent[i] = newContent;
                m_lastModified[i] = modified;

                qDebug() << "Detected external change to buffer" << i;
                emit bufferChangedExternally(i, newContent);
            }
            else
            {
                // Just timestamp changed, not content
                m_lastModified[i] = modified;
            }
        }
    }
}
