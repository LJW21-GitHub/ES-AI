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


from PyQt6.QtMultimedia import (QAudio, QAudioDevice, QAudioFormat,
        QMediaDevices)
from PyQt6.QtWidgets import QApplication, QTableWidgetItem, QMainWindow

from ui_audiodevicesbase import Ui_AudioDevicesBase


class AudioDevicesBase(QMainWindow, Ui_AudioDevicesBase):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.setupUi(self)


class AudioTest(AudioDevicesBase):

    def __init__(self, parent=None):
        super().__init__(parent)

        self.devices = QMediaDevices(self)
        self.deviceInfo = QAudioDevice()
        self.settings = QAudioFormat()
        self.mode = QAudioDevice.Mode.Input

        self.testButton.clicked.connect(self.test)
        self.modeBox.activated.connect(self.modeChanged)
        self.deviceBox.activated.connect(self.deviceChanged)
        self.sampleRateSpinBox.valueChanged.connect(self.sampleRateChanged)
        self.channelsSpinBox.valueChanged.connect(self.channelChanged)
        self.sampleFormatBox.activated.connect(self.sampleFormatChanged)
        self.populateTableButton.clicked.connect(self.populateTable)

        self.devices.audioInputsChanged.connect(self.updateAudioDevices)
        self.devices.audioOutputsChanged.connect(self.updateAudioDevices)

        self.modeBox.setCurrentIndex(0)
        self.modeChanged(0)
        self.deviceBox.setCurrentIndex(0)
        self.deviceChanged(0)

    def test(self):
        self.testResult.clear()

        if not self.deviceInfo.isNull():
            if self.deviceInfo.isFormatSupported(self.settings):
                self.testResult.setText("Success")
                self.nearestSampleRate.setText("")
                self.nearestChannel.setText("")
                self.nearestSampleFormat.setText("")
            else:
                nearest = self.deviceInfo.preferredFormat()
                self.testResult.setText("Failed")
                self.nearestSampleRate.setText(str(nearest.sampleRate()))
                self.nearestChannel.setText(str(nearest.channelCount()))
                self.nearestSampleFormat.setText(
                        self.sampleFormatToString(nearest.sampleFormat()))
        else:
            self.testResult.setText("No Device")

    sampleFormatMap = {
        QAudioFormat.SampleFormat.UInt8: "Unsigned 8 bit",
        QAudioFormat.SampleFormat.Int16: "Signed 16 bit",
        QAudioFormat.SampleFormat.Int32: "Signed 32 bit",
        QAudioFormat.SampleFormat.Float: "Float"
    }

    @classmethod
    def sampleFormatToString(cls, sampleFormat):
        return cls.sampleFormatMap.get(sampleFormat, "Unknown")

    def updateAudioDevices(self):
        self.deviceBox.clear()

        devs = self.devices.audioInputs() if self.mode is QAudioDevice.Mode.Input else self.devices.audioOutputs()

        for deviceInfo in devs:
            self.deviceBox.addItem(deviceInfo.description(), deviceInfo)

    def modeChanged(self, idx):
        self.testResult.clear()

        self.mode = QAudioDevice.Mode.Input if idx == 0 else QAudioDevice.Mode.Output
        self.updateAudioDevices()
        self.deviceBox.setCurrentIndex(0)
        self.deviceChanged(0)

    def deviceChanged(self, idx):
        self.testResult.clear()

        if self.deviceBox.count() == 0:
            return

        self.deviceInfo = self.deviceBox.itemData(idx)

        minSampleRate = self.deviceInfo.minimumSampleRate()
        maxSampleRate = self.deviceInfo.maximumSampleRate()

        self.sampleRateSpinBox.clear()
        self.sampleRateSpinBox.setMinimum(minSampleRate)
        self.sampleRateSpinBox.setMaximum(maxSampleRate)
        sampleValue = max(minSampleRate, min(48000, maxSampleRate))
        self.settings.setSampleRate(sampleValue)

        minChannelCount = self.deviceInfo.minimumChannelCount()
        maxChannelCount = self.deviceInfo.maximumChannelCount()

        self.channelsSpinBox.clear()
        self.channelsSpinBox.setMinimum(minChannelCount)
        self.channelsSpinBox.setMaximum(maxChannelCount)
        channelValue = max(minChannelCount, min(2, maxChannelCount))
        self.settings.setChannelCount(channelValue)

        self.sampleFormatBox.clear()
        sampleFormats = self.deviceInfo.supportedSampleFormats()
        self.sampleFormatBox.addItems(
                [self.sampleFormatToString(st) for st in sampleFormats])
        if len(sampleFormats) != 0:
            self.settings.setSampleFormat(sampleFormats[0])

        self.allFormatsTable.clearContents()

    def populateTable(self):
        row = 0

        for sampleFormat in self.deviceInfo.supportedSampleFormats():
            self.allFormatsTable.setRowCount(row + 1)

            sampleTypeItem = QTableWidgetItem(
                    self.sampleFormatToString(sampleFormat))
            self.allFormatsTable.setItem(row, 0, sampleTypeItem)

            sampleRateItem = QTableWidgetItem(
                    "{0} - {1}".format(self.deviceInfo.minimumSampleRate(),
                            self.deviceInfo.maximumSampleRate()))
            self.allFormatsTable.setItem(row, 1, sampleRateItem)

            channelsItem = QTableWidgetItem(
                    "{0} - {1}".format(self.deviceInfo.minimumChannelCount(),
                            self.deviceInfo.maximumChannelCount()))
            self.allFormatsTable.setItem(row, 2, channelsItem)

            row += 1

    def sampleRateChanged(self, value):
        self.settings.setSampleRate(value)

    def channelChanged(self, value):
        self.settings.setChannelCount(value)

    def sampleFormatChanged(self, idx):
        formats = self.deviceInfo.supportedSampleFormats()
        self.settings.setSampleFormat(formats[idx])


if __name__ == '__main__':

    import sys

    app = QApplication(sys.argv)
    app.setApplicationName("Audio Device Test")

    audio = AudioTest()
    audio.show()

    sys.exit(app.exec())
