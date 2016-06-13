#include "shotgunner.h"
#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  Shotgunner w;
  w.show();

  return a.exec();
}
