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

from PyQt6.QtCore import (QByteArray, QDataStream, QIODevice, QMimeData,
        QPoint, Qt)
from PyQt6.QtGui import QColor, QDrag, QPainter, QPixmap
from PyQt6.QtWidgets import QApplication, QFrame, QHBoxLayout, QLabel, QWidget


class DragWidget(QFrame):
    def __init__(self, parent=None):
        super().__init__(parent)

        images_dir = os.path.join(os.path.dirname(__file__), 'images')
        self._images_dir = images_dir

        self.setMinimumSize(200, 200)
        self.setFrameStyle(QFrame.Shadow.Sunken | QFrame.Shape.StyledPanel)
        self.setAcceptDrops(True)

        boatIcon = QLabel(self)
        boatIcon.setPixmap(QPixmap(os.path.join(images_dir, 'boat.png')))
        boatIcon.move(10, 10)
        boatIcon.show()
        boatIcon.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose)

        carIcon = QLabel(self)
        carIcon.setPixmap(QPixmap(os.path.join(images_dir, 'car.png')))
        carIcon.move(100, 10)
        carIcon.show()
        carIcon.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose)

        houseIcon = QLabel(self)
        houseIcon.setPixmap(QPixmap(os.path.join(images_dir, 'house.png')))
        houseIcon.move(10, 80)
        houseIcon.show()
        houseIcon.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose)

    def dragEnterEvent(self, event):
        if event.mimeData().hasFormat('application/x-dnditemdata'):
            if event.source() is self:
                event.setDropAction(Qt.DropAction.MoveAction)
                event.accept()
            else:
                event.acceptProposedAction()
        else:
            event.ignore()

    dragMoveEvent = dragEnterEvent

    def dropEvent(self, event):
        if event.mimeData().hasFormat('application/x-dnditemdata'):
            itemData = event.mimeData().data('application/x-dnditemdata')
            dataStream = QDataStream(itemData, QIODevice.OpenModeFlag.ReadOnly)

            pixmap = QPixmap()
            offset = QPoint()
            dataStream >> pixmap >> offset

            newIcon = QLabel(self)
            newIcon.setPixmap(pixmap)
            newIcon.move(event.position().toPoint() - offset)
            newIcon.show()
            newIcon.setAttribute(Qt.WidgetAttribute.WA_DeleteOnClose)

            if event.source() is self:
                event.setDropAction(Qt.DropAction.MoveAction)
                event.accept()
            else:
                event.acceptProposedAction()
        else:
            event.ignore()

    def mousePressEvent(self, event):
        child = self.childAt(event.position().toPoint())
        if not child:
            return

        pixmap = child.pixmap()

        itemData = QByteArray()
        dataStream = QDataStream(itemData, QIODevice.OpenModeFlag.WriteOnly)
        dataStream << pixmap << QPoint(event.position().toPoint() - child.pos())

        mimeData = QMimeData()
        mimeData.setData('application/x-dnditemdata', itemData)

        drag = QDrag(self)
        drag.setMimeData(mimeData)
        drag.setPixmap(pixmap)
        drag.setHotSpot(event.position().toPoint() - child.pos())

        tempPixmap = QPixmap(pixmap)
        painter = QPainter()
        painter.begin(tempPixmap)
        painter.fillRect(pixmap.rect(), QColor(127, 127, 127, 127))
        painter.end()

        child.setPixmap(tempPixmap)

        if drag.exec(Qt.DropAction.CopyAction | Qt.DropAction.MoveAction, Qt.DropAction.CopyAction) == Qt.DropAction.MoveAction:
            child.close()
        else:
            child.show()
            child.setPixmap(pixmap)


if __name__ == '__main__':

    import sys

    app = QApplication(sys.argv)

    mainWidget = QWidget()
    horizontalLayout = QHBoxLayout()
    horizontalLayout.addWidget(DragWidget())
    horizontalLayout.addWidget(DragWidget())

    mainWidget.setLayout(horizontalLayout)
    mainWidget.setWindowTitle("Draggable Icons")
    mainWidget.show()

    sys.exit(app.exec())
