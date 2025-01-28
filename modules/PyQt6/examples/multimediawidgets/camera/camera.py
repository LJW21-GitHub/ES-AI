#############################################################################
##
## Copyright (C) 2021 Riverbank Computing Limited.
## Copyright (C) 2020 The Qt Company Ltd.
## All rights reserved.
##
## This file is part of the examples of PyQt.
##
## $QT_BEGIN_LICENSE:BSD$
## You may use this file under the terms of the BSD license as follows:
##
## "Redistribution and use in source and binary forms, with or without
## modification, are permitted provided that the following conditions are
## met:
##   * Redistributions of source code must retain the above copyright
##     notice, this list of conditions and the following disclaimer.
##   * Redistributions in binary form must reproduce the above copyright
##     notice, this list of conditions and the following disclaimer in
##     the documentation and/or other materials provided with the
##     distribution.
##   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
##     the names of its contributors may be used to endorse or promote
##     products derived from this software without specific prior written
##     permission.
##
## THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
## OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
## SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
## LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
## DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
## THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
## (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
## OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
## $QT_END_LICENSE$
##
#############################################################################


import os

from PyQt6.QtCore import QDateTime, QDir, qFuzzyCompare, Qt, QTimer
from PyQt6.QtGui import QAction, QActionGroup, QIcon, QImage, QPixmap
from PyQt6.QtMultimedia import (QAudioInput, QCamera, QImageCapture,
        QMediaFormat, QMediaDevices, QMediaCaptureSession, QMediaMetaData,
        QMediaRecorder)
from PyQt6.QtWidgets import (QApplication, QDialog, QDialogButtonBox,
        QFormLayout, QHBoxLayout, QLineEdit, QMainWindow, QMessageBox,
        QPushButton, QScrollArea, QVBoxLayout, QWidget)

from ui_camera import Ui_Camera
from ui_imagesettings import Ui_ImageSettingsUi
from ui_videosettings import Ui_VideoSettingsUi


class ImageSettings(QDialog):

    def __init__(self, imageCapture, parent=None):
        super().__init__(parent)

        self.ui = Ui_ImageSettingsUi()
        self.imagecapture = imageCapture

        self.ui.setupUi(self)

        self.ui.imageCodecBox.addItem("Default image format", "")
        for f in self.imagecapture.supportedFormats():
            description = QImageCapture.fileFormatDescription(f)
            self.ui.imageCodecBox.addItem(
                    QImageCapture.fileFormatName(f) + ": " + description, f)

        self.ui.imageQualitySlider.setRange(
                QImageCapture.Quality.VeryLowQuality.value,
                QImageCapture.Quality.VeryHighQuality.value)

        self.ui.imageResolutionBox.addItem("Default resolution")
        supportedResolutions = self.imagecapture.captureSession().camera().cameraDevice().photoResolutions()
        for resolution in supportedResolutions:
            self.ui.imageResolutionBox.addItem(
                    "%dx%d" % (resolution.width(), resolution.height()),
                    resolution)

        self.selectComboBoxItem(self.ui.imageCodecBox,
                self.imagecapture.fileFormat())
        self.selectComboBoxItem(self.ui.imageResolutionBox,
                self.imagecapture.resolution())
        self.ui.imageQualitySlider.setValue(self.imagecapture.quality().value)

    def applyImageSettings(self):
        self.imagecapture.setFileFormat(self.boxValue(self.ui.imageCodecBox))
        self.imagecapture.setQuality(
                QImageCapture.Quality(self.ui.imageQualitySlider.value()))
        self.imagecapture.setResolution(
                self.boxValue(self.ui.imageResolutionBox))

    @staticmethod
    def boxValue(box):
        idx = box.currentIndex()
        if idx == -1:
            return None

        return box.itemData(idx)

    @staticmethod
    def selectComboBoxItem(box, value):
        for i in range(box.count()):
            if box.itemData(i) == value:
                box.setCurrentIndex(i)
                break


class VideoSettings(QDialog):

    def __init__(self, mediaRecorder, parent=None):
        super().__init__(parent)

        self.ui = Ui_VideoSettingsUi()
        self.mediaRecorder = mediaRecorder

        self.ui.setupUi(self)

        self.ui.audioCodecBox.addItem("Default audio codec",
                QMediaFormat.AudioCodec.Unspecified)
        for ac in QMediaFormat().supportedAudioCodecs(QMediaFormat.ConversionMode.Encode):
            description = QMediaFormat.audioCodecDescription(ac)
            self.ui.audioCodecBox.addItem(
                    QMediaFormat.audioCodecName(ac) + ": " + description, ac)

        audioDevice = self.mediaRecorder.captureSession().audioInput().device()
        self.ui.audioSampleRateBox.setRange(audioDevice.minimumSampleRate(),
                audioDevice.maximumSampleRate())

        self.ui.videoCodecBox.addItem("Default video codec",
                QMediaFormat.VideoCodec.Unspecified)
        for vc in QMediaFormat().supportedVideoCodecs(QMediaFormat.ConversionMode.Encode):
            description = QMediaFormat.videoCodecDescription(vc)
            self.ui.videoCodecBox.addItem(
                    QMediaFormat.videoCodecName(vc) + ": " + description, vc)

        self.ui.videoResolutionBox.addItem("Default")
        supportedResolutions = self.mediaRecorder.captureSession().camera().cameraDevice().photoResolutions()
        for resolution in supportedResolutions:
            self.ui.videoResolutionBox.addItem(
                    "%dx%d" % (resolution.width(), resolution.height()),
                    resolution)

        self.ui.videoFramerateBox.addItem("Default")
        #supportedFrameRates, _ = self.mediaRecorder.supportedFrameRates()
        #for rate in supportedFrameRates:
        #    self.ui.videoFramerateBox.addItem("%0.2f" % rate, rate)

        self.ui.containerFormatBox.addItem("Default container",
                QMediaFormat.FileFormat.UnspecifiedFormat)
        for fmt in QMediaFormat().supportedFileFormats(QMediaFormat.ConversionMode.Encode):
            description = QMediaFormat.fileFormatDescription(fmt)
            self.ui.containerFormatBox.addItem(
                    QMediaFormat.fileFormatName(fmt) + ": " + description, fmt)

        self.ui.qualitySlider.setRange(
                QMediaRecorder.Quality.VeryLowQuality.value,
                QMediaRecorder.Quality.VeryHighQuality.value)

        mediaFormat = self.mediaRecorder.mediaFormat()
        self.selectComboBoxItem(self.ui.containerFormatBox,
                mediaFormat.fileFormat())
        self.selectComboBoxItem(self.ui.audioCodecBox,
                mediaFormat.audioCodec())
        self.selectComboBoxItem(self.ui.videoCodecBox,
                mediaFormat.videoCodec())

        self.ui.qualitySlider.setValue(self.mediaRecorder.quality().value)
        self.ui.audioSampleRateBox.setValue(
                self.mediaRecorder.audioSampleRate())
        self.selectComboBoxItem(self.ui.videoResolutionBox,
                self.mediaRecorder.videoResolution())

        for i in range(1, self.ui.videoFramerateBox.count()):
            itemRate = self.ui.videoFramerateBox.itemData(i)
            if qFuzzyCompare(itemRate, self.mediaRecorder.videoFrameRate()):
                self.ui.videoFramerateBox.setCurrentIndex(i)
                break

    def applySettings(self):
        mediaFormat = QMediaFormat()
        mediaFormat.setFileFormat(self.boxValue(self.ui.containerFormatBox))
        mediaFormat.setAudioCodec(self.boxValue(self.ui.audioCodecBox))
        mediaFormat.setVideoCodec(self.boxValue(self.ui.videoCodecBox))

        self.mediaRecorder.setMediaFormat(mediaFormat)
        self.mediaRecorder.setQuality(
                QMediaRecorder.Quality(self.ui.qualitySlider.value()))
        self.mediaRecorder.setAudioSampleRate(
                self.ui.audioSampleRateBox.value())
        self.mediaRecorder.setVideoResolution(
                boxValue(self.ui.videoResolutionBox))
        self.mediaRecorder.setVideoFrameRate(
                boxValue(self.ui.videoFramerateBox))

    @staticmethod
    def boxValue(box):
        idx = box.currentIndex()
        if idx == -1:
            return None

        return box.itemData(idx)

    @staticmethod
    def selectComboBoxItem(box, value):
        for i in range(box.count()):
            if box.itemData(i) == value:
                box.setCurrentIndex(i)
                break


class MetaDataDialog(QDialog):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.metaDataFields = {}

        metaDataLayout = QFormLayout()

        for key in QMediaMetaData.Key:
            label = QMediaMetaData.metaDataKeyToString(key)
            field = QLineEdit()
            self.metaDataFields[key] = field

            if key is QMediaMetaData.Key.ThumbnailImage:
                openThumbnail = QPushButton("Open",
                        clicked=self.openThumbnailImage)
                layout = QHBoxLayout()
                layout.addWidget(field)
                layout.addWidget(openThumbnail)
                metaDataLayout.addRow(label, layout)

            elif key is QMediaMetaData.Key.CoverArtImage:
                openCoverArt = QPushButton("Open",
                        clicked=self.openCoverArtImage)
                layout = QHBoxLayout()
                layout.addWidget(field)
                layout.addWidget(openCoverArt)
                metaDataLayout.addRow(label, layout)

            else:
                if key is QMediaMetaData.Key.Title:
                    field.setText("PyQt Camera Example")
                elif key is QMediaMetaData.Key.Author:
                    field.setText("Riverbank Computing")
                elif key is QMediaMetaData.Key.Date:
                    field.setText(QDateTime.currentDateTime().toString())

                metaDataLayout.addRow(label, field)

        viewport = QWidget()
        viewport.setLayout(metaDataLayout)
        scrollArea = QScrollArea()
        scrollArea.setWidget(viewport)
        dialogLayout = QVBoxLayout()
        dialogLayout.addWidget(scrollArea)

        buttonBox = QDialogButtonBox(
                QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
        dialogLayout.addWidget(buttonBox)
        self.setLayout(dialogLayout)

        self.setWindowTitle("Set Metadata")
        self.resize(400, 300)

        buttonBox.accepted.connect(self.accept)
        buttonBox.rejected.connect(self.reject)

    def openThumbnailImage(self):
        fileName, _ = QFileDialog.getOpenFileName(self, "Open Image",
                QDir.currentPath(), "Image Files (*.png *.jpg *.bmp)")

        if fileName:
            self.metaDataFields[QMediaMetaData.Key.ThumbnailImage].setText(
                    fileName)

    def openCoverArtImage(self):
        fileName, _ = QFileDialog.getOpenFileName(self, "Open Image",
                QDir.currentPath(), "Image Files (*.png *.jpg *.bmp)")

        if fileName:
            self.metaDataFields[QMediaMetaData.Key.CoverArtImage].setText(
                    fileName)


class Camera(QMainWindow):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.ui = Ui_Camera()

        self.devices = QMediaDevices()
        self.captureSession = QMediaCaptureSession()
        self.camera = None
        self.imageCapture = None
        self.mediaRecorder = None
        self.isCapturingImage = False
        self.applicationExiting = False
        self.doImageCapture = True
        self.metaDataDialog = None

        self.ui.setupUi(self)

        self.ui.takeImageButton.setIcon(
                QIcon(
                        os.path.join(os.path.dirname(__file__), 'images',
                                'shutter.svg')))

        self.audioInput = QAudioInput()
        self.captureSession.setAudioInput(self.audioInput)

        self.videoDevicesGroup = QActionGroup(self)
        self.videoDevicesGroup.setExclusive(True)

        self.updateCameras()
        self.devices.videoInputsChanged.connect(self.updateCameras)

        self.videoDevicesGroup.triggered.connect(self.updateCameraDevice)
        self.ui.captureWidget.currentChanged.connect(self.updateCaptureMode)

        self.ui.metaDataButton.clicked.connect(self.showMetaDataDialog)

        self.setCamera(QMediaDevices.defaultVideoInput())

    def setCamera(self, cameraDevice):
        self.camera = QCamera(cameraDevice)
        self.captureSession.setCamera(self.camera)

        self.camera.activeChanged.connect(self.updateCameraActive)
        self.camera.errorOccurred.connect(self.displayCameraError)

        self.mediaRecorder = QMediaRecorder()
        self.captureSession.setRecorder(self.mediaRecorder)
        self.mediaRecorder.recorderStateChanged.connect(
                self.updateRecorderState)

        self.imageCapture = QImageCapture()
        self.captureSession.setImageCapture(self.imageCapture)

        self.mediaRecorder.durationChanged.connect(self.updateRecordTime)
        self.mediaRecorder.errorChanged.connect(self.displayRecorderError)

        self.ui.exposureCompensation.valueChanged.connect(
                self.setExposureCompensation)

        self.captureSession.setVideoOutput(self.ui.viewfinder)

        self.updateCameraActive(self.camera.isActive())
        self.updateRecorderState(self.mediaRecorder.recorderState())

        self.imageCapture.readyForCaptureChanged.connect(self.readyForCapture)
        self.imageCapture.imageCaptured.connect(self.processCapturedImage)
        self.imageCapture.imageSaved.connect(self.imageSaved)
        self.imageCapture.errorOccurred.connect(self.displayCaptureError)
        self.readyForCapture(self.imageCapture.isReadyForCapture())

        self.updateCaptureMode()
        self.camera.start()

    def keyPressEvent(self, event):
        if event.isAutoRepeat():
            return

        if event.key() is Qt.Key.Key_CameraFocus:
            self.displayViewfinder()
            self.camera.searchAndLock()
            event.accept()
        elif event.key() is Qt.Key.Key_Camera:
            if self.doImageCapture:
                self.takeImage()
            elif self.mediaRecorder.recorderState() is QMediaRecorder.RecorderState.RecordingState:
                self.stop()
            else:
                self.record()

            event.accept()
        else:
            super().keyPressEvent(event)

    def updateRecordTime(self):
        msg = "Recorded %d sec" % (self.mediaRecorder.duration() // 1000)
        self.ui.statusbar.showMessage(msg)

    def processCapturedImage(self, requestId, img):
        scaledImage = img.scaled(self.ui.viewfinder.size(),
                Qt.AspectRatioMode.KeepAspectRatio,
                Qt.TransformationMode.SmoothTransformation)

        self.ui.lastImagePreviewLabel.setPixmap(QPixmap.fromImage(scaledImage))

        self.displayCapturedImage()
        QTimer.singleShot(4000, self.displayViewfinder)

    def configureCaptureSettings(self):
        if self.doImageCapture:
            self.configureImageSettings()
        else:
            self.configureVideoSettings()

    def configureVideoSettings(self):
        settingsDialog = VideoSettings(self.mediaRecorder)
        settingsDialog.setWindowFlags(
                settingsDialog.windowFlags() & ~Qt.WindowType.WindowContextHelpButtonHint)

        if settingsDialog.exec():
            settingsDialog.applySettings()

    def configureImageSettings(self):
        settingsDialog = ImageSettings(self.imageCapture)
        settingsDialog.setWindowFlags(
                settingsDialog.windowFlags() & ~Qt.WindowType.WindowContextHelpButtonHint)

        if settingsDialog.exec():
            settingsDialog.applyImageSettings()

    def record(self):
        self.mediaRecorder.record()
        self.updateRecordTime()

    def pause(self):
        self.mediaRecorder.pause()

    def stop(self):
        self.mediaRecorder.stop()

    def setMuted(self, muted):
        self.captureSession.audioInput().setMuted(muted)

    def takeImage(self):
        self.isCapturingImage = True
        self.imageCapture.captureToFile()

    def displayCaptureError(self, id, error, errorString):
        QMessageBox.warning(self, "Image Capture Error", errorString)
        self.isCapturingImage = False

    def startCamera(self):
        self.camera.start()

    def stopCamera(self):
        self.camera.stop()

    def updateCaptureMode(self):
        tabIndex = self.ui.captureWidget.currentIndex()
        self.doImageCapture = (tabIndex == 0)

    def updateCameraActive(self, active):
        if active:
            self.ui.actionStartCamera.setEnabled(False)
            self.ui.actionStopCamera.setEnabled(True)
            self.ui.captureWidget.setEnabled(True)
            self.ui.actionSettings.setEnabled(True)
        else:
            self.ui.actionStartCamera.setEnabled(True)
            self.ui.actionStopCamera.setEnabled(False)
            self.ui.captureWidget.setEnabled(False)
            self.ui.actionSettings.setEnabled(False)

    def updateRecorderState(self, state):
        if state is QMediaRecorder.RecorderState.StoppedState:
            self.ui.recordButton.setEnabled(True)
            self.ui.pauseButton.setEnabled(True)
            self.ui.stopButton.setEnabled(False)
            self.ui.metaDataButton.setEnabled(True)
        elif state is QMediaRecorder.RecorderState.PausedState:
            self.ui.recordButton.setEnabled(True)
            self.ui.pauseButton.setEnabled(False)
            self.ui.stopButton.setEnabled(True)
            self.ui.metaDataButton.setEnabled(False)
        elif state is QMediaRecorder.RecorderState.RecordingState:
            self.ui.recordButton.setEnabled(False)
            self.ui.pauseButton.setEnabled(True)
            self.ui.stopButton.setEnabled(True)
            self.ui.metaDataButton.setEnabled(False)

    def setExposureCompensation(self, index):
        self.camera.setExposureCompensation(index * 0.5)

    def displayRecorderError(self):
        if self.mediaRecorder.error() is not QMediaRecorder.Error.NoError:
            QMessageBox.warning(self, "Capture Error",
                    self.mediaRecorder.errorString())

    def displayCameraError(self):
        if self.camera.error() is not QCamers.Error.NoError:
            QMessageBox.warning(self, "Camera Error",
                    self.camera.errorString())

    def updateCameraDevice(self, action):
        self.setCamera(action.data())

    def displayViewfinder(self):
        self.ui.stackedWidget.setCurrentIndex(0)

    def displayCapturedImage(self):
        self.ui.stackedWidget.setCurrentIndex(1)

    def readyForCapture(self, ready):
        self.ui.takeImageButton.setEnabled(ready)

    def imageSaved(self, id, fileName):
        self.ui.statusbar.showMessage(
                "Captured \"%s\"" % QDir.toNativeSeparators(fileName))
        self.isCapturingImage = False

        if self.applicationExiting:
            self.close()

    def closeEvent(self, event):
        if self.isCapturingImage:
            self.setEnabled(False)
            self.applicationExiting = True
            event.ignore()
        else:
            event.accept()

    def updateCameras(self):
        self.ui.menuDevices.clear()

        for cameraDevice in QMediaDevices.videoInputs():
            videoDeviceAction = QAction(cameraDevice.description(),
                    self.videoDevicesGroup)
            videoDeviceAction.setCheckable(True)
            videoDeviceAction.setData(cameraDevice)

            if cameraDevice == QMediaDevices.defaultVideoInput():
                videoDeviceAction.setChecked(True)

            self.ui.menuDevices.addAction(videoDeviceAction)

    def showMetaDataDialog(self):
        if self.metaDataDialog is None:
            self.metaDataDialog = MetaDataDialog(self)

        self.metaDataDialog.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose,
                False)

        if self.metaDataDialog.exec() is QDialog.DialogCode.Accepted:
            self.saveMetaData()

    def saveMetaData(self):
        data = QMediaMetaData()

        for key in QMediaMetaData.Key:
            val = self.metaDataDialog.metaDataFields[key].text()
            if val:
                if key in (QMediaMetaData.Key.CoverArtImage, QMediaMetaData.Key.ThumbnailImage):
                    val = QImage(val)
                elif key is QMediaMetaData.Key.Date:
                    val = QDateTime.fromString(val)

                data.insert(key, val)

        self.mediaRecorder.setMetaData(data)


if __name__ == '__main__':

    import sys

    app = QApplication(sys.argv)

    camera = Camera()
    camera.show()

    sys.exit(app.exec())
