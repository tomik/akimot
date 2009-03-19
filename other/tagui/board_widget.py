from PyQt4 import QtGui
from PyQt4 import QtCore

ANIMATE_ONE = 0.2

MAX_MOVE_INTERVAL = 1500
STEP_INTERVAL = 300

ERROR = -1
BSIDE = 8 
BSIZE = 64
COL_EMPTY = 2
COLORS = ['Gold', 'Silver', '']
PIECE_EMPTY_STR = 'Empty'
PIECES = {'r': 'Rabbit', 'c': 'Cat', 'd': 'Dog', 'h': 'Horse', 'm': 'Camel', 'e': 'Elephant', ' ' : PIECE_EMPTY_STR}
PIECES_NUM = ['r', 'c', 'd', 'h', 'm', 'e', ' ']

def scale_pixmap(pixmap, size, ar=None):
    return pixmap.scaled(size.width(), size.height(), ar and ar or QtCore.Qt.KeepAspectRatio)

class BoardWidget(QtGui.QWidget):
    def __init__(self, widget):
        self.pos = None
        self.empty_square_pixmap = QtGui.QPixmap('img/Empty.gif')

        self.mover = QtCore.QTimeLine(MAX_MOVE_INTERVAL)
        self.mover.setCurveShape(self.mover.LinearCurve)
        self.connect(self.mover, QtCore.SIGNAL("frameChanged(int)"), self.animate_move)

        widget = super(BoardWidget, self).__init__(widget) 

        self.square_width = 0
        self.square_height = 0
        self.repaint_index = None

        return widget
        
    def compute_rec(self, index):
        rec  = self.geometry()
        left = rec.x() + (index % BSIDE) * self.square_width
        top  = rec.y() + (index / BSIDE) * self.square_height

        return QtCore.QRect(left, top, self.square_width, self.square_height) 
        

    def resizeEvent(self, e):
        rec = self.geometry()
        self.board_pixmap = scale_pixmap(QtGui.QPixmap("img/BoardStoneBig.jpg"), rec)
        rec.setHeight(self.board_pixmap.height()) 
        rec.setWidth(self.board_pixmap.width()) 
        self.square_width  = rec.width()  / float(BSIDE) 
        self.square_height = rec.height() / float(BSIDE) 

        self.repaint()

    def paintEvent(self, e):
        painter = QtGui.QPainter(self)
        painter.drawPixmap(0, 0, self.board_pixmap)

        if self.pos:
            if self.repaint_index: 
                c = self.pos.to_short_str()[1:-1][self.repaint_index]
                player = (c == ' ' and COL_EMPTY or (c.islower() and 1 or 0))
                self.draw_square(painter, player, c.lower(), self.repaint_index)
                self.repaint_index = None
            else: #repaint all
                for index, c in enumerate(self.pos.to_short_str()[1:-1]):
                    player = (c == ' ' and COL_EMPTY or (c.islower() and 1 or 0))
                    self.draw_square(painter, player, c.lower(), index)

    def start_move_animation(self, steps):
        self.steps = steps
        self.mover.setDuration(min(len(steps) * STEP_INTERVAL, MAX_MOVE_INTERVAL)) 
        self.mover.setCurrentTime(0)
        self.mover.setFrameRange(0, len(steps))
        self.mover.start()
            
    def animate_move(self, frame):
        if frame == 0:
            return
        (_, from_in), (_, to_in), step_piece, step_color = self.steps[frame - 1]
        to_in = BSIZE - to_in - 1
        from_in = BSIZE - from_in - 1
        self.pos = self.pos.do_move([self.steps[frame -1]])
        #erase previous
        if from_in != ERROR: #not an init => clear previous position
            index = from_in
            self.repaint_index = index
            self.repaint(self.compute_rec(index))

        if to_in == ERROR: #trapped
            index = to_in
        else: #ok
            index = to_in

        self.repaint_index = index
        self.repaint(self.compute_rec(index))

    def update_pos(self, pos):
        self.pos = pos
        self.mover.stop()
        self.repaint()

    def draw_square(self, painter, player, piece, index):
        s = "img/%s%s.gif" % (COLORS[player], PIECES[piece])
        pixmap = scale_pixmap(QtGui.QPixmap(s), self.compute_rec(index))
        painter.drawPixmap(self.compute_rec(index).x(), self.compute_rec(index).y(), pixmap)

