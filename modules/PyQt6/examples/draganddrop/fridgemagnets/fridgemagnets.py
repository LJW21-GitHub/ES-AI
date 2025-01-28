#############################################################################
##
## Copyright (C) 2021 Riverbank Computing Limited.
## Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

from PyQt6.QtCore import (QByteArray, QDataStream, QFile, QIODevice, QMimeData,
        QPoint, QRect, QRectF, Qt, QTextStream)
from PyQt6.QtGui import (QDrag, QFont, QFontMetrics, QImage, QPainter,
        QPalette, QPixmap, qRgba)
from PyQt6.QtWidgets import QApplication, QLabel, QWidget


class DragLabel(QLabel):
    def __init__(self, text, parent):
        super().__init__(parent)

        metric = QFontMetrics(self.font())
        size = metric.size(Qt.TextFlag.TextSingleLine, text)

        image = QImage(size.width() + 12, size.height() + 12,
                QImage.Format.Format_ARGB32_Premultiplied)
        image.fill(qRgba(0, 0, 0, 0))

        font = QFont()
        font.setStyleStrategy(QFont.StyleStrategy.ForceOutline)

        painter = QPainter()
        painter.begin(image)
        painter.setRenderHint(QPainter.RenderHint.Antialiasing)
        painter.setBrush(Qt.GlobalColor.white)
        painter.drawRoundedRect(
                QRectF(0.5, 0.5, image.width()-1, image.height()-1),
                25, 25, Qt.SizeMode.RelativeSize)

        painter.setFont(font)
        painter.setBrush(Qt.GlobalColor.black)
        painter.drawText(QRect(QPoint(6, 6), size),
                Qt.AlignmentFlag.AlignCenter, text)
        painter.end()

        self.setPixmap(QPixmap.fromImage(image))
        self.labelText = text

    def mousePressEvent(self, event):
        itemData = QByteArray()
        dataStream = QDataStream(itemData, QIODevice.OpenModeFlag.WriteOnly)
        dataStream << QByteArray(self.labelText.encode('utf8')) << QPoint(event.position().toPoint() - self.rect().topLeft())

        mimeData = QMimeData()
        mimeData.setData('application/x-fridgemagnet', itemData)
        mimeData.setText(self.labelText)

        drag = QDrag(self)
        drag.setMimeData(mimeData)
        drag.setHotSpot(event.position().toPoint() - self.rect().topLeft())
        drag.setPixmap(self.pixmap())

        self.hide()

        if drag.exec(Qt.DropAction.MoveAction | Qt.DropAction.CopyAction, Qt.DropAction.CopyAction) == Qt.DropAction.MoveAction:
            self.close()
        else:
            self.show()


class DragWidget(QWidget):
    def __init__(self, parent=None):
        super().__init__(parent)

        dictionaryFile = QFile(
                os.path.join(os.path.dirname(__file__), 'words.txt'))
        dictionaryFile.open(QFile.OpenModeFlag.ReadOnly)

        x = 5
        y = 5

        for word in QTextStream(dictionaryFile).readAll().split():
            wordLabel = DragLabel(word, self)
            wordLabel.move(x, y)
            wordLabel.show()
            x += wordLabel.width() + 2
            if x >= 245:
                x = 5
                y += wordLabel.height() + 2

        newPalette = self.palette()
        newPalette.setColor(QPalette.ColorRole.Window, Qt.GlobalColor.white)
        self.setPalette(newPalette)

        self.setMinimumSize(400, max(200, y))
        self.setWindowTitle("Fridge Magnets")
        self.setAcceptDrops(True)

    def dragEnterEvent(self, event):
        if event.mimeData().hasFormat('application/x-fridgemagnet'):
            if event.source() in self.children():
                event.setDropAction(Qt.DropAction.MoveAction)
                event.accept()
            else:
                event.acceptProposedAction()
        elif event.mimeData().hasText():
            event.acceptProposedAction()
        else:
            event.ignore()

    dragMoveEvent = dragEnterEvent

    def dropEvent(self, event):
        if event.mimeData().hasFormat('application/x-fridgemagnet'):
            mime = event.mimeData()
            itemData = mime.data('application/x-fridgemagnet')
            dataStream = QDataStream(itemData, QIODevice.OpenModeFlag.ReadOnly)

            text = QByteArray()
            offset = QPoint()
            dataStream >> text >> offset

            text = str(text, encoding='utf8')

            newLabel = DragLabel(text, self)
            newLabel.move(event.position().toPoint() - offset)
            newLabel.show()

            if event.source() in self.children():
                event.setDropAction(Qt.DropAction.MoveAction)
                event.accept()
            else:
                event.acceptProposedAction()
        elif event.mimeData().hasText():
            pieces = event.mimeData().text().split()
            position = event.position().toPoint()

            for piece in pieces:
                newLabel = DragLabel(piece, self)
                newLabel.move(position)
                newLabel.show()

                position += QPoint(newLabel.width(), 0)

            event.acceptProposedAction()
        else:
            event.ignore()


if __name__ == '__main__':

    import sys

    app = QApplication(sys.argv)
    window = DragWidget()
    window.show()
    sys.exit(app.exec())
