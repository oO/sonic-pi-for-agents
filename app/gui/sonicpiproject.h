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

#pragma once

#include <QString>
#include <QDir>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QMap>
#include <QDateTime>

/**
 * SonicPiProject manages a folder-based project with file-backed buffers.
 *
 * Project structure:
 *   project-folder/
 *   ├── buffer.0.spi
 *   ├── buffer.1.spi
 *   ├── ...
 *   └── buffer.9.spi
 *
 * Files are the source of truth. Buffers in the GUI are just editor views.
 */
class SonicPiProject : public QObject
{
    Q_OBJECT

public:
    explicit SonicPiProject(QObject* parent = nullptr);
    ~SonicPiProject();

    /**
     * Create a new project in the given folder.
     * Creates 10 buffer files (buffer.0.spi - buffer.9.spi) with empty content.
     */
    static SonicPiProject* create(const QString& folderPath, QObject* parent = nullptr);

    /**
     * Load an existing project from the given folder.
     * Expects 10 buffer files to exist.
     */
    static SonicPiProject* load(const QString& folderPath, QObject* parent = nullptr);

    /**
     * Get the full path to a buffer file.
     * @param bufferId Buffer number (0-9)
     * @return Full filesystem path to buffer.{bufferId}.spi
     */
    QString getBufferFilePath(int bufferId) const;

    /**
     * Get the project folder path.
     */
    QString getProjectPath() const { return m_projectPath; }

    /**
     * Read buffer content from file.
     * @param bufferId Buffer number (0-9)
     * @return File content as QString
     */
    QString readBuffer(int bufferId) const;

    /**
     * Write buffer content to file.
     * @param bufferId Buffer number (0-9)
     * @param content Content to write
     */
    bool writeBuffer(int bufferId, const QString& content);

    /**
     * Check if project folder and all buffer files exist.
     */
    bool isValid() const;

    /**
     * Get number of buffers (always 10 for now).
     */
    int getBufferCount() const { return 10; }

signals:
    /**
     * Emitted when a buffer file changes externally (not from this process).
     * @param bufferId Buffer number that changed
     * @param newContent New file content
     */
    void bufferChangedExternally(int bufferId, const QString& newContent);

private:
    explicit SonicPiProject(const QString& projectPath, QObject* parent = nullptr);

    void initFileWatcher();
    void checkForExternalChanges();
    QString bufferFileName(int bufferId) const;

private slots:
    void onFileChanged(const QString& path);
    void onPollTimeout();

private:
    QString m_projectPath;
    QFileSystemWatcher* m_fileWatcher;
    QTimer* m_pollTimer;

    // Track last known content to detect changes
    QMap<int, QString> m_lastKnownContent;
    QMap<int, QDateTime> m_lastModified;

    // Track writes from this process to avoid false positives
    QMap<int, QDateTime> m_lastLocalWrite;
};
