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


from math import pi, sin
from struct import pack

from PyQt6.QtCore import QByteArray, QIODevice, Qt, QTimer, qWarning
from PyQt6.QtMultimedia import (QAudio, QAudioDevice, QAudioFormat, QAudioSink,
        QMediaDevices)
from PyQt6.QtWidgets import (QApplication, QComboBox, QHBoxLayout, QLabel,
        QMainWindow, QPushButton, QSlider, QVBoxLayout, QWidget)


class Generator(QIODevice):

    def __init__(self, format, durationUs, sampleRate):
        super().__init__()

        self.m_pos = 0
        self.m_buffer = QByteArray()

        if format.isValid():
            self.generateData(format, durationUs, sampleRate)

    def start(self):
        self.open(QIODevice.OpenModeFlag.ReadOnly)

    def stop(self):
        self.m_pos = 0
        self.close()

    def generateData(self, format, durationUs, sampleRate):
        packFormat = ''

        sampleFormat = format.sampleFormat()

        if sampleFormat is QAudioFormat.SampleFormat.UInt8:
            scaler = lambda x: ((1.0 + x) / 2 * 255)
            packFormat = 'B'
        elif sampleFormat is QAudioFormat.SampleFormat.Int16:
            scaler = lambda x: x * 32767
            packFormat = 'h'
        elif sampleFormat is QAudioFormat.SampleFormat.Int32:
            scaler = lambda x: x * 2147483647
            packFormat = 'i'
        elif sampleFormat is QAudioFormat.SampleFormat.Float:
            scaler = lambda x: x
            packFormat = 'f'

        assert(packFormat != '')

        channelBytes = format.bytesPerSample()
        sampleBytes = format.channelCount() * channelBytes
        length = format.bytesForDuration(durationUs)

        assert(length % sampleBytes == 0)

        self.m_buffer.clear()
        sampleIndex = 0
        factor = 2 * pi * sampleRate / format.sampleRate()

        while length != 0:
            x = sin((sampleIndex % format.sampleRate()) * factor)
            packed = pack(packFormat, int(scaler(x)))

            for _ in range(format.channelCount()):
                self.m_buffer.append(packed)
                length -= channelBytes

            sampleIndex += 1

    def readData(self, maxlen):
        data = QByteArray()
        total = 0

        while maxlen > total:
            chunk = min(self.m_buffer.size() - self.m_pos, maxlen - total)
            data.append(self.m_buffer.mid(self.m_pos, chunk))
            self.m_pos = (self.m_pos + chunk) % self.m_buffer.size()
            total += chunk

        return data.data()

    def writeData(self, data):
        return 0

    def bytesAvailable(self):
        return self.m_buffer.size() + super().bytesAvailable()


class AudioTest(QMainWindow):

    DurationSeconds = 1
    ToneSampleRateHz = 600

    def __init__(self):
        super().__init__()

        self.m_devices = QMediaDevices(self)
        self.m_pushTimer = QTimer(self)
        self.m_pullMode = True
        self.m_generator = None
        self.m_audioOutput = None

        self.initializeWindow()
        self.initializeAudio(self.m_devices.defaultAudioOutput())

    def initializeWindow(self):
        layout = QVBoxLayout()

        self.m_deviceBox = QComboBox(activated=self.deviceChanged)
        defaultDeviceInfo = self.m_devices.defaultAudioOutput()
        self.m_deviceBox.addItem(defaultDeviceInfo.description(),
                defaultDeviceInfo)

        for deviceInfo in self.m_devices.audioOutputs():
            if deviceInfo != defaultDeviceInfo:
                self.m_deviceBox.addItem(deviceInfo.description(), deviceInfo)

        layout.addWidget(self.m_deviceBox)

        self.m_modeButton = QPushButton(clicked=self.toggleMode)
        layout.addWidget(self.m_modeButton)

        self.m_suspendResumeButton = QPushButton(
                clicked=self.toggleSuspendResume)
        layout.addWidget(self.m_suspendResumeButton)

        volumeBox = QHBoxLayout()
        volumeLabel = QLabel("Volume:")
        self.m_volumeSlider = QSlider(Qt.Orientation.Horizontal, minimum=0,
                maximum=100, singleStep=10, valueChanged=self.volumeChanged)
        volumeBox.addWidget(volumeLabel)
        volumeBox.addWidget(self.m_volumeSlider)

        layout.addLayout(volumeBox)

        window = QWidget()
        window.setLayout(layout)

        self.setCentralWidget(window)

    def initializeAudio(self, deviceInfo):
        format = deviceInfo.preferredFormat()

        self.m_generator = Generator(format, self.DurationSeconds * 1000000,
                self.ToneSampleRateHz)

        self.m_audioOutput = QAudioSink(deviceInfo, format)

        self.m_generator.start()

        initialVolume = QAudio.convertVolume(self.m_audioOutput.volume(),
                QAudio.VolumeScale.LinearVolumeScale,
                QAudio.VolumeScale.LogarithmicVolumeScale)
        self.m_volumeSlider.setValue(int(initialVolume * 100))

        self.toggleMode()

    def deviceChanged(self, index):
        self.m_generator.stop()
        self.m_audioOutput.stop()

        try:
            # This will raise an exception if the are no connections.
            self.m_audioOutput.disconnect()
        except:
            pass

        self.initializeAudio(self.m_deviceBox.itemData(index))

    def volumeChanged(self, value):
        linearVolume = QAudio.convertVolume(value / 100.0,
                QAudio.VolumeScale.LinearVolumeScale,
                QAudio.VolumeScale.LogarithmicVolumeScale)

        self.m_audioOutput.setVolume(linearVolume)

    def toggleMode(self):
        self.m_pushTimer.stop()
        self.m_audioOutput.stop()
        self.toggleSuspendResume()

        if self.m_pullMode:
            self.m_modeButton.setText("Enable push mode")
            self.m_audioOutput.start(self.m_generator)
        else:
            self.m_modeButton.setText("Enable pull mode")
            io = self.m_audioOutput.start()

            try:
                # This will raise an exception if the are no connections.
                self.m_pushTimer.timeout.disconnect()
            except:
                pass

            self.m_pushTimer.timeout.connect(lambda: self.handlePushTimer(io))
            self.m_pushTimer.start(10)

        self.m_pullMode = not self.m_pullMode

    def handlePushTimer(self, io):
        if self.m_audioOutput.state() is QAudio.State.StoppedState:
            return

        buffer = self.m_generator.read(self.m_audioOutput.bytesFree())
        if buffer:
            io.write(buffer)

    def toggleSuspendResume(self):
        if self.m_audioOutput.state() in (QAudio.State.SuspendedState, QAudio.State.StoppedState):
            self.m_audioOutput.resume()
            self.m_suspendResumeButton.setText("Suspend playback")
        elif self.m_audioOutput.state() is QAudio.State.ActiveState:
            self.m_audioOutput.suspend()
            self.m_suspendResumeButton.setText("Resume playback")


if __name__ == '__main__':

    import sys

    app = QApplication(sys.argv)
    app.setApplicationName("Audio Output Test")

    audio = AudioTest()
    audio.show()

    sys.exit(app.exec())
