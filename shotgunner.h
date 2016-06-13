/* --------------------------------------------------------

   Project:
      Shotgunner

    Purpose:
      Simulates the flight of a round ball

   Author:
      Stephen C. Wardlaw, M.D.
      IVDx Consulting
      Highrock
      Lyme, CT 06371
      <swardlaw@ivdx.com>

   Edit History:
      30 March, 2011

 -------------------------------------------------------- */
#ifndef Shotgunner_H
#define Shotgunner_H

#include <QtWidgets/QMainWindow>
#include "ui_shotgunner.h"

class Shotgunner : public QMainWindow
{
	Q_OBJECT

public:
explicit Shotgunner(QWidget *parent = 0);

	~Shotgunner();

private slots:
	void
		altitudeChanged(int index);

	void
		shotChanged(int);

	void
		mvChanged();

	void
		tempChanged();

	void
		weightChanged();

	void
		windChanged();

	void
		fireClicked();

private:	// Methods

double
	calcBallWeight();

void
	calcRelDensity();

void
	calculate();

void
	doOutputLine(int index);

double
	lapsedTemp(double alt);

QString
	makeFileName();

double	// Retardation value
	rValue(double vel);

void
	showInput();

void
	showLine(QString line);

void
	showOutput();

private:	// Data
	QVector<double> mVelocity;
	QVector<double>	mDrop;
	QVector<double>	mDrift;
	QVector<double> mTof;


	double	mShotSize;
	double	mShotDensity;   // Density of shot type
	double	mWeight;
	double	mMuzzleVelocity;
	double	mXWind;
	double	mSlope;
	double	mTemp;
	double	mRelDensity;	// Relative air density
	double	mMaxYards;	// Distance for >= 1 fpe.

	Ui::Shotgunner ui;
};

#endif // Shotgunner_H
