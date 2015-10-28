/*
Copyright (c) 2015 rikyoz <rik20@live.it>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
<http://www.gnu.org/licenses/>.
*/
#include <QCryptographicHash>
#include <QDesktopWidget>
#include <QFileDialog>
#include <QMimeDatabase>
#include <QDateTime>
#include <QFileIconProvider>

#ifdef QT_DEBUG
#include <QDebug>
#endif

#include "mainwindow.hpp"
#include "about.hpp"
#include "qhasher.hpp"

using namespace std;

#define UPPERCASE_SETTING QStringLiteral("show_uppercase")

#ifdef Q_OS_WIN
/* Needed to read correct file properties on NTFS file systems,
 * see http://doc.qt.io/qt-5/qfiledevice.html#Permission-enum */
extern Q_CORE_EXPORT int qt_ntfs_permission_lookup;
#endif

QString humanReadableSize( qint64 size ) {
    QString byteSize =  QString::number( size ) + " bytes";

    if ( size < 1024 ) return byteSize;

    QStringList list = { "KB", "MB", "GB", "TB" };

    QStringListIterator iter( list );
    float fsize = size;
    while ( fsize >= 1024.0 && iter.hasNext() ) {
        iter.next();
        fsize /= 1024.0;
    }
    return QString("%1 %2 (%3)").arg( fsize, 0, 'f', 2 ).arg( iter.peekPrevious() ).arg( byteSize );
}

MainWindow::MainWindow( QWidget* parent ) : QMainWindow( parent ),
                                            settings( "settings.ini", QSettings::IniFormat ) {
    setupUi( this );
    setFixedSize( this->size() );
    setGeometry( QStyle::alignedRect( Qt::LeftToRight, Qt::AlignCenter, this->size(),
                                            qApp->desktop()->availableGeometry() ) );
    setWindowTitle( "MrHash v" + QString::number( MAJOR_VER ) + "." + QString::number( MINOR_VER ) );

    connect( actionAboutQt, SIGNAL( triggered() ), qApp, SLOT( aboutQt() ) );

    actionUseUppercase->setChecked( settings.value( UPPERCASE_SETTING, false ).toBool() );
}

MainWindow::~MainWindow() {}

void MainWindow::closeEvent( QCloseEvent * ) {
    settings.setValue( UPPERCASE_SETTING, actionUseUppercase->isChecked() );
}

void MainWindow::on_actionInformazioni_su_Hasher_triggered() {
    About abt_dlg;
    abt_dlg.exec();
}

void MainWindow::on_actionEsci_triggered() {
    close();
}

void MainWindow::on_actionUseUppercase_toggled( bool useUppercase ){
    foreach( QLineEdit* lineEdit, findChildren<QLineEdit*>() ) {
        if ( lineEdit != filePathEdit ) {
            QString text = lineEdit->text();
            lineEdit->setText( useUppercase ? text.toUpper() : text.toLower() );
        }
    }
}

void MainWindow::on_plainTextEdit_textChanged() {
    QString text = plainTextEdit->toPlainText();
    calculateHashes( text.toUtf8(), actionUseUppercase->isChecked() );
}

void MainWindow::on_pushButton_clicked() {
    QFileDialog fileDialog(this);
    if ( fileDialog.exec() == QFileDialog::Accepted ) {
        if ( fileDialog.selectedFiles().size() == 0 ) return;

        filePathEdit->setText( fileDialog.selectedFiles()[0] );
        readFileInfo( fileDialog.selectedFiles()[0] );

        calculateFileHashes( fileDialog.selectedFiles()[0] );
    }
}

void MainWindow::on_tabWidget_currentChanged( int index ) {
    if ( index != 0 )
        on_plainTextEdit_textChanged();
    else if ( filePathEdit->text().isEmpty() ) {
        //no file selected, no hash to show
        foreach( QLineEdit* lineEdit, findChildren<QLineEdit*>() ) {
            if ( lineEdit != filePathEdit )
                lineEdit->clear();
        }
    } else
        calculateFileHashes( filePathEdit->text() );
}

inline QString MainWindow::boolToStr( bool value ) { return value ? tr("yes") : tr("no"); }

void MainWindow::readFileInfo( QString filePath ) {
#ifdef Q_OS_WIN
    qt_ntfs_permission_lookup++;
#endif

    QFileInfo fileInfo( filePath );
    pathLabel->setText( fileInfo.absolutePath() );
    nameLabel->setText( fileInfo.fileName() );
    extLabel->setText( fileInfo.suffix() );

    QMimeDatabase mimeDatabase;
    mimeLabel->setText( mimeDatabase.mimeTypeForFile( fileInfo, QMimeDatabase::MatchExtension ).name() );
    ctypeLabel->setText( mimeDatabase.mimeTypeForFile( fileInfo, QMimeDatabase::MatchContent ).name() );

    sizeLabel->setText( humanReadableSize( fileInfo.size() ) );
    lastReadLabel->setText( fileInfo.lastRead().toString( Qt::DefaultLocaleLongDate ) );
    lastChangeLabel->setText( fileInfo.lastModified().toString( Qt::DefaultLocaleLongDate ) );
    creationLabel->setText( fileInfo.created().toString( Qt::DefaultLocaleLongDate ) );
    hiddenLabel->setText( boolToStr( fileInfo.isHidden() ) );
    readableLabel->setText( boolToStr( fileInfo.isReadable() ) );
    writableLabel->setText( boolToStr( fileInfo.isWritable() ) );
    executableLabel->setText( boolToStr( fileInfo.isExecutable() ) );
    ownerLabel->setText( fileInfo.owner() );

    QFileIconProvider iconProvider;
    QIcon icon = iconProvider.icon( fileInfo );
    fileIconLabel->setPixmap( icon.pixmap( icon.availableSizes().last() ) );

#ifdef Q_OS_WIN
    qt_ntfs_permission_lookup--;
#endif
}

void MainWindow::calculateFileHashes( QString fileName ) {
    QFile file( fileName );
    if ( file.open( QFile::ReadOnly ) )
        calculateHashes( file.readAll(), actionUseUppercase->isChecked() );
}

void MainWindow::calculateHashes( QByteArray content, bool show_uppercase ) {
    crc16edit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::CRC16 ) );
    crc32edit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::CRC32 ) );
    crc64edit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::CRC64 ) );
    md4edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Md4 ) );
    md5edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Md5 ) );
    sha1edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha1 ) );
    sha224edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha224 ) );
    sha256edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha256 ) );
    sha384edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha384 ) );
    sha512edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha512 ) );
    sha3224edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha3_224 ) );
    sha3256edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha3_256 ) );
    sha3384edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha3_384 ) );
    sha3512edit->setText( QHashCalculator::hash( content, show_uppercase, QCryptographicHash::Sha3_512 ) );
    tigeredit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::TIGER ) );
    ripemdedit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::RIPEMD160 ) );
    haval128edit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::HAVAL128 ) );
    haval160edit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::HAVAL160 ) );
    haval192edit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::HAVAL192 ) );
    haval224edit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::HAVAL224 ) );
    haval256edit->setText( QHashCalculator::hash( content, show_uppercase, QHashAlgorithm::HAVAL256 ) );
    base64edit->setText( content.toBase64() );

    sha384edit->setCursorPosition(0);
    sha512edit->setCursorPosition(0);
    sha3384edit->setCursorPosition(0);
    sha3512edit->setCursorPosition(0);
}
