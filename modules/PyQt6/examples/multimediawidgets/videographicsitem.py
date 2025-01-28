#############################################################################
##
## Copyright (C) 2021 Riverbank Computing Limited.
## Copyright (C) 2017 The Qt Company Ltd.
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


from PyQt6.QtCore import QDir, QSizeF, QStandardPaths, Qt, QUrl
from PyQt6.QtGui import QTransform
from PyQt6.QtMultimedia import QMediaPlayer
from PyQt6.QtMultimediaWidgets import QGraphicsVideoItem
from PyQt6.QtWidgets import (QApplication, QFileDialog, QGraphicsScene,
        QGraphicsView, QHBoxLayout, QPushButton, QSlider, QStyle, QVBoxLayout,
        QWidget)


class VideoPlayer(QWidget):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.mediaPlayer = QMediaPlayer(self)

        screenGeometry = self.screen().availableSize()
        self.videoItem = QGraphicsVideoItem()
        self.videoItem.setSize(
                QSizeF(screenGeometry.width() / 3, screenGeometry.height() / 2))

        scene = QGraphicsScene(self)
        graphicsView = QGraphicsView(scene)

        scene.addItem(self.videoItem)

        rotateSlider = QSlider(Qt.Orientation.Horizontal)
        rotateSlider.setToolTip("Rotate Video")
        rotateSlider.setRange(-180,  180)
        rotateSlider.setValue(0)
        rotateSlider.valueChanged.connect(self.rotateVideo)

        openButton = QPushButton("Open...")
        openButton.clicked.connect(self.openFile)

        self.playButton = QPushButton()
        self.playButton.setEnabled(False)
        self.playButton.setIcon(
                self.style().standardIcon(QStyle.StandardPixmap.SP_MediaPlay))
        self.playButton.clicked.connect(self.play)

        self.positionSlider = QSlider(Qt.Orientation.Horizontal)
        self.positionSlider.setRange(0, 0)
        self.positionSlider.sliderMoved.connect(self.setPosition)

        controlLayout = QHBoxLayout()
        controlLayout.setContentsMargins(0, 0, 0, 0)
        controlLayout.addWidget(openButton)
        controlLayout.addWidget(self.playButton)
        controlLayout.addWidget(self.positionSlider)

        layout = QVBoxLayout(self)
        layout.addWidget(graphicsView)
        layout.addWidget(rotateSlider)
        layout.addLayout(controlLayout)

        self.mediaPlayer.setVideoOutput(self.videoItem)
        self.mediaPlayer.playbackStateChanged.connect(self.mediaStateChanged)
        self.mediaPlayer.positionChanged.connect(self.positionChanged)
        self.mediaPlayer.durationChanged.connect(self.durationChanged)

    def sizeHint(self):
        return ((self.videoItem.size() * 3.0) / 2.0).toSize()

    def isPlayerAvailable(self):
        return self.mediaPlayer.isAvailable()

    def openFile(self):
        fileDialog = QFileDialog(self)
        fileDialog.setAcceptMode(QFileDialog.AcceptMode.AcceptOpen)
        fileDialog.setWindowTitle("Open Movie")

        try:
            location = QStandardPaths.standardLocations(
                    QStandardPaths.StandardLocation.MoviesLocation)[0]
        except IndexError:
            location = QDir.homePath()

        fileDialog.setDirectory(location)

        if fileDialog.exec() == QFileDialog.DialogCode.Accepted:
            self.load(fileDialog.selectedUrls()[0])

    def load(self, url):
        self.mediaPlayer.setSource(url)
        self.playButton.setEnabled(True)

    def play(self):
        if self.mediaPlayer.playbackState() is QMediaPlayer.PlaybackState.PlayingState:
            self.mediaPlayer.pause()
        else:
            self.mediaPlayer.play()

    def mediaStateChanged(self, state):
        if state is QMediaPlayer.PlaybackState.PlayingState:
            icon = QStyle.StandardPixmap.SP_MediaPause
        else:
            icon = QStyle.StandardPixmap.SP_MediaPlay

        self.playButton.setIcon(self.style().standardIcon(icon))

    def positionChanged(self, position):
        self.positionSlider.setValue(position)

    def durationChanged(self, duration):
        self.positionSlider.setRange(0, duration)

    def setPosition(self, position):
        self.mediaPlayer.setPosition(position)

    def rotateVideo(self, angle):
        x = self.videoItem.boundingRect().width() / 2.0
        y = self.videoItem.boundingRect().height() / 2.0

        self.videoItem.setTransform(
                QTransform().translate(x, y).rotate(angle).translate(-x, -y))


if __name__ == '__main__':

    import sys

    app = QApplication(sys.argv)

    player = VideoPlayer()
    player.show()

    sys.exit(app.exec())
