#!/usr/bin/python

import sys
from os.path import isfile
from PyQt4 import QtGui
from PyQt4 import QtCore

from main_window import Ui_MainWindow

sys.path.append('..')

from aei.aei import StdioEngine, EngineController 
from aei import board 

ERROR = -1

AKIMOT_CMD = 'bots/akimot -c bots/default.cfg -l'

from board_widget import MODE_PLAY, MODE_REPLAY

class Tagui(QtGui.QMainWindow):

    def __init__(self, parent=None):
        QtGui.QWidget.__init__(self, parent)
        self.ui = Ui_MainWindow()
        self.ui.setupUi(self)
        self.board = self.ui.board
        self.record = self.ui.record
        self.record.last_row = self.record.currentRow()

        self.engines = {}
        self.pos, _ = self.get_pos(0)
        self.mode = None

        self.load_position('example_pos.ari')

    def on_actionLoadGame_triggered(self,checked=None):
        if checked is None: return 
        self.load_record()

    def on_actionLoadPosition_triggered(self,checked=None):
        if checked is None: return 
        self.load_position()

    def on_actionSavePosition_triggered(self,checked=None):
        if checked is None: return 
        self.save_position()

    def on_actionNewGame_triggered(self,checked=None):
        if checked is None: return 
        self.board.update_pos(self.make_empty_pos())
        self.record.clear()
        self.board.mode = MODE_PLAY
        

    def on_actionExit_triggered(self,checked=None):
        quit()

    def load_record(self, fn=None):
        if not fn:
            fd = QtGui.QFileDialog(self)
            fn = fd.getOpenFileName()
        if isfile(fn):
            self.record.clear()
            self.record.addItem('record')
            self.record.addItems(open(fn).readlines())
            self.record.setSelectionMode(self.record.SingleSelection)
            self.record.setCurrentRow(0)
            self.board.mode = MODE_REPLAY

    def load_position(self, fn=None):
        if not fn:
            fd = QtGui.QFileDialog(self)
            fn = fd.getOpenFileName()
        if isfile(fn):
            self.record.clear()
            _, self.pos = board.parse_long_pos(open(fn).readlines())
            self.board.update_pos(self.pos)
            self.board.mode = MODE_PLAY

    def save_position(self, fn=None):
        if not self.pos:
            return 

        print self.pos.to_short_str()
        
        if not fn:
            dialog = QtGui.QFileDialog(self)
            fn = dialog.getSaveFileName()
            
            fd = open(fn, 'w')
            fd.write(self.pos.to_long_str())
            fd.close()

    def add_engine(self, name, cmd):
        self.engines[name] = EngineController(StdioEngine(cmd, None))
        
    def on_button_back_clicked(self, checked=None):
        if checked is None: return
        row = self.record.currentRow() 
        assert row > 0, "cannot go back"
        self.record.setCurrentRow(row - 1)
        
    def on_button_forward_clicked(self, checked=None):
        if checked is None: return

    def add_engine(self, name, cmd):
        self.engines[name] = EngineController(StdioEngine(cmd, None))
        
    def on_button_back_clicked(self, checked=None):
        if checked is None: return
        row = self.record.currentRow() 
        assert row > 0, "cannot go back"
        self.record.setCurrentRow(row - 1)
        
    def on_button_forward_clicked(self, checked=None):
        if checked is None: return
        row = self.record.currentRow() 
        assert row < self.record.count() - 1, "cannot go forward"
        self.record.blockSignals(True) 
        row += 1
        self.record.setCurrentRow(row)
        self.record.blockSignals(False) 
        self.start_move_animation(row)
        self.record_update()

    def on_record_currentRowChanged(self):
        row = self.record.currentRow()
        if row - self.record.last_row == 1: 
            self.start_move_animation(row)
            self.record_update()
            return
        self.pos, last = self.get_pos(row)
        self.board.update_pos(self.pos)
        self.record_update()

    def record_update(self):
        row = self.record.currentRow()
        self.record.last_row = row
        self.ui.button_back.setEnabled(True)
        self.ui.button_forward.setEnabled(True)
        if row == 0: 
            self.ui.button_back.setEnabled(False)
        if row >= self.record.count() - 1:
            self.ui.button_forward.setEnabled(False)

    def make_empty_pos(self):
        bitboards = list([list(x) for x in board.BLANK_BOARD])
        return board.Position(board.COL_GOLD, 4, bitboards)

    def get_pos(self, row, separate_last_move=False):
        moves = map(lambda x: str(x.text()).strip(), [self.record.item(index) for index in xrange(1, row + 1)])
        pos = self.make_empty_pos()
        if row == 0: #moves == ['record'] or moves == []:
            return pos, []

        if not separate_last_move:
            moves.append(' ')

        moves = map(lambda x: x[x.find(' ') + 1:], moves)
        for m in moves[:-1]: 
            try: 
                steps = board.parse_move(m)
                pos = pos.do_move(steps)
            except ValueError:
                break

        return pos, moves[-1]

    def start_move_animation(self, move_num):
        self.pos, last_move = self.get_pos(move_num, separate_last_move=True)
        self.board.update_pos(self.pos)
        if last_move: 
            steps = board.parse_move(last_move)
            self.board.start_move_animation(steps)

    def start_search(self, name):
        self.engines[name].newgame()
        #self.engines[name].setoption('tcmove', '%s' % self.engine.ttm )
        self.engines[name].setposition(self.pos)
        self.engine.go() 

        while True:
            resp = self.engine.get_response()
            if resp.type == "bestmove":
                resp.message = resp.move
            if resp.type == "log":
                if resp.message.split(' ')[-1] == "over":
                    break

    def __del__(self): 
        for engine in self.engines.values():
            engine.quit()
            engine.cleanup()


if __name__ == '__main__':
    app = QtGui.QApplication(sys.argv)
    window = Tagui()
    window.show()
    app.exec_()

