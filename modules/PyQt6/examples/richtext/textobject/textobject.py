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


from PyQt6.QtCore import QObject, QSizeF
from PyQt6.QtGui import QTextCharFormat, QTextFormat, QTextObjectInterface
from PyQt6.QtWidgets import (QApplication, QHBoxLayout, QLabel, QLineEdit,
        QPushButton, QTextEdit, QVBoxLayout, QWidget)
from PyQt6.QtSvg import QSvgRenderer


class SvgTextObject(QObject, QTextObjectInterface):

    def intrinsicSize(self, doc, posInDocument, format):
        renderer = QSvgRenderer(format.property(Window.SvgFileName))
        size = renderer.defaultSize()

        if size.height() > 25:
            size *= 25.0 / size.height()

        return QSizeF(size)

    def drawObject(self, painter, rect, doc, posInDocument, format):
        renderer = QSvgRenderer(format.property(Window.SvgFileName))
        renderer.render(painter, rect)


class Window(QWidget):

    SvgTextFormat = QTextFormat.ObjectTypes.UserObject + 1

    SvgFileName = 1

    def __init__(self):
        super().__init__()

        self.setupGui()
        self.setupTextObject()

        self.setWindowTitle("Text Object Example")

    def insertTextObject(self):
        svgCharFormat = QTextCharFormat()
        svgCharFormat.setObjectType(Window.SvgTextFormat)
        svgCharFormat.setProperty(Window.SvgFileName,
                self.fileNameLineEdit.text())

        cursor = self.textEdit.textCursor()
        cursor.insertText('\N{object replacement character}', svgCharFormat)
        self.textEdit.setTextCursor(cursor)

    def setupTextObject(self):
        svgInterface = SvgTextObject(self)
        self.textEdit.document().documentLayout().registerHandler(Window.SvgTextFormat, svgInterface)

    def setupGui(self):
        fileNameLabel = QLabel("Svg File Name:")
        self.fileNameLineEdit = QLineEdit()
        insertTextObjectButton = QPushButton("Insert Image")

        self.fileNameLineEdit.setText('heart.svg')
        insertTextObjectButton.clicked.connect(self.insertTextObject)

        bottomLayout = QHBoxLayout()
        bottomLayout.addWidget(fileNameLabel)
        bottomLayout.addWidget(self.fileNameLineEdit)
        bottomLayout.addWidget(insertTextObjectButton)

        self.textEdit = QTextEdit()

        mainLayout = QVBoxLayout()
        mainLayout.addWidget(self.textEdit)
        mainLayout.addLayout(bottomLayout)

        self.setLayout(mainLayout)


if __name__ == '__main__':

    import sys

    app = QApplication(sys.argv)
    window = Window()
    window.show()
    sys.exit(app.exec())
