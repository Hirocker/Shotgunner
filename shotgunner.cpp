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
#include "shotgunner.h"
#include <QtCore>
#include <math.h>

const double grains_per_pound = 7000;// Grains per pound
const double dens_lead = 0.4093;    // Density of lead in lbs/cu.in.
const double dens_chilled = 0.401;  // Chilled shot
const double dens_hard = 0.393;     // Hard shot
const double dens_steel = 0.284;    // Steel shot
const double dens_bismuth = 0.347;  // Bismuth shot
const double dens_tIron = 0.372;    // Tungsten-iron shot
const double dens_tMatrix = 0.383;  // Tungsten-matrix shot
const double dens_hevi = 0.433;     // Heavi-Shotconst double dens_lead = 0.4093;	// Density of lead in lbs/cu.in.
const double dens_iron = 0.2529; // Cast iron
const double dens_zinc = 0.2578;    // Zinc
const double dens_aluminum = 0.09798;   // Aluminum
const double _pi_ = 3.14159;
const double g_English = 32.174;	// g - in English units ft/sec/sec
const double mph_to_fps	= 1.46667;	// MPH to FPS conversion
const double in_per_foot = 12.0;		// Inches per foot
const double drag_coeff	= 0.0755;	// Arbitrary drag coeff. This makes the British Retardation data match real-world data. DON'T CHANGE!
const double BC = 1.0;	// Relative ballistic coefficient. Tweak this if you must, not 'drag_coeff'
const int data_interval = 1;	// Data every x yards
const double std_temp_f = 70.0;	// 'Standard' sea-level temperature
const double temp_lapse_rate = 3.55E-3;	// Environmental lapse rate in deg F/ft
const int d_spc = 10;		// Output line data spacing


// --------------------------------------------------------
//			CONSTRUCTOR AND DESTRUCTOR
// --------------------------------------------------------
Shotgunner::Shotgunner(QWidget *parent)
	: QMainWindow(parent)
{
	ui.setupUi(this);

	connect(ui.mEditWeight,SIGNAL(editingFinished()),this,SLOT(weightChanged()));
	connect(ui.mEditVelocity,SIGNAL(editingFinished()),this,SLOT(mvChanged()));
	connect(ui.mEditWind,SIGNAL(editingFinished()),this,SLOT(windChanged()));
	connect(ui.mEditTemp,SIGNAL(editingFinished()),this,SLOT(tempChanged()));
	connect(ui.mRunSimulation,SIGNAL(clicked()),this,SLOT(fireClicked()));
	connect(ui.mActShowForce,SIGNAL(activated()),this,SLOT(createPressureCurve()));

	mXWind = 0.0;
	mMuzzleVelocity = 1350.0;
	mTemp = 70.0;	// Seventy def F as std temp

	// Set up the altitude selection
	ui.mAltitude->addItem("Sea Level",QVariant(0));
	double alt = 500.0;
	while(alt <= 10000.0) {
		ui.mAltitude->addItem(QString("%1").arg(alt,0,'f',0),alt);
		alt += 500.0;
	}
	// Now, connect it...
	connect(ui.mAltitude,SIGNAL(currentIndexChanged(int)),this,SLOT(altitudeChanged(int)));

	// Set up the shot size table
	ui.mShotSize->addItem("#11",0.062);
	ui.mShotSize->addItem("#10",0.070);
	ui.mShotSize->addItem("#9 1/2",0.075);
	ui.mShotSize->addItem("#9",0.080);
	ui.mShotSize->addItem("#8 1/2",0.085);
	ui.mShotSize->addItem("#8",0.090);
	ui.mShotSize->addItem("#7 1/2",0.095);
	ui.mShotSize->addItem("#7",0.10);
	ui.mShotSize->addItem("#6",0.11);
	ui.mShotSize->addItem("#5",0.12);
	ui.mShotSize->addItem("#4",0.13);
	ui.mShotSize->addItem("#3",0.14);
	ui.mShotSize->addItem("#2",0.15);
	ui.mShotSize->addItem("#1",0.16);
	ui.mShotSize->addItem("BB",0.177);
	ui.mShotSize->addItem("4 Buck",0.24);
	ui.mShotSize->addItem("3 Buck",0.25);
	ui.mShotSize->addItem("1 Buck",0.30);
	ui.mShotSize->addItem("0 Buck",0.32);
	ui.mShotSize->addItem("00 Buck",0.34);
	ui.mShotSize->addItem("000 Buck",0.36);

	ui.mShotSize->setCurrentIndex(6);
	connect(ui.mShotSize,SIGNAL(currentIndexChanged(int)),this,SLOT(shotChanged(int)));
	mShotSize = ui.mShotSize->currentData().toDouble();

	ui.mShotType->addItem("Steel",dens_steel);
	ui.mShotType->addItem("Bismuth",dens_bismuth);
	ui.mShotType->addItem("Tung-Iron",dens_tIron);
	ui.mShotType->addItem("Tung-Mtx",dens_tMatrix);
	ui.mShotType->addItem("Hard",dens_hard);
	ui.mShotType->addItem("Chilled",dens_chilled);
	ui.mShotType->addItem("Hevi-Shot",dens_hevi);
	ui.mShotType->setCurrentIndex(5);


	connect(ui.mShotType,SIGNAL(currentIndexChanged(int)),this,SLOT(shotChanged(int)));
	mShotDensity = ui.mShotType->currentData().toDouble();
	mWeight = calcBallWeight();
	showInput();
	calcRelDensity();
}

Shotgunner::~Shotgunner()
{

}
// --------------------------------------------------------
//			SLOTS
// --------------------------------------------------------
void
Shotgunner::shotChanged(int)
{
  mShotSize = ui.mShotSize->currentData().toDouble();
  mShotDensity = ui.mShotType->currentData().toDouble();
  mWeight = calcBallWeight();
  showInput();
}
// --------------------------------------------------------
void
Shotgunner::altitudeChanged(int index)
{
	double alt = ui.mAltitude->itemData(index).toDouble();
	mTemp = lapsedTemp(alt);
	calcRelDensity();
	showInput();
}
// --------------------------------------------------------
void
Shotgunner::mvChanged()
{
	bool ok;
	double val;

	if(ui.mEditVelocity->isModified()) {
		val = ui.mEditVelocity->text().toDouble(&ok);
		if(!ok) {
			showLine(QString("<b>Error</b> MV is not a valid number"));
			showInput();
			return;
		}
		if(val < 100.0) {
			showLine(QString("<b>Error</b> MV must be greater than 100 fps"));
			showInput();
			return;
		}
		if(val > 4000.0) {
			showLine(QString("<b>Error</b> MV must be less than 4000 fps"));
			showInput();
			return;
		}
		mMuzzleVelocity = val;
	}
	showInput();
}
// --------------------------------------------------------
void
Shotgunner::tempChanged()
{
	bool ok;
	double val;

	if(ui.mEditTemp->isModified()) {
		val = ui.mEditTemp->text().toDouble(&ok);
		if(!ok) {
			showLine(QString("<b>Error</b> Temperature is not a valid number"));
			showInput();
			return;
		}
		if(val < -10.0) {
			showLine(QString("<b>Error</b> Temp. must be greater than -10 deg."));
			showInput();
			return;
		}
		if(val > 120.0) {
			showLine(QString("<b>Error</b> Temp. must be less than 120 deg."));
			showInput();
			return;
		}
		mTemp = val;
	}
	calcRelDensity();
	showInput();
}
// --------------------------------------------------------
void
Shotgunner::weightChanged()
{
	bool ok;
	double val;

	if(ui.mEditWeight->isModified()) {
		val = ui.mEditWeight->text().toDouble(&ok);
		if(!ok) {
			showLine(QString("<b>Error</b> Weight is not a valid number"));
			showInput();
			return;
		}
		if(val < 1.0) {
			showLine(QString("<b>Error</b> Weight must be greater than 1 gr."));
			showInput();
			return;
		}
		if(val > 7000.0) {
			showLine(QString("<b>Error</b> Weight must be less than 7000 gr."));
			showInput();
			return;
		}
		mWeight = val;
	}
	showInput();
}
// --------------------------------------------------------
void
Shotgunner::windChanged()
{
	bool ok;
	double val;

	if(ui.mEditWind->isModified()) {
		val = ui.mEditWind->text().toDouble(&ok);
		if(!ok) {
			showLine(QString("<b>Error</b> Wind is not a valid number"));
			showInput();
			return;
		}
		if(val < 0.0) {
			showLine(QString("<b>Error</b> Wind must be greater than 0"));
			showInput();
			return;
		}
		if(val > 60.0) {
			showLine(QString("<b>Error</b> Wind must be less than 60 mph"));
			showInput();
			return;
		}
		mXWind = val;
	}
	showInput();
}
// --------------------------------------------------------
void
Shotgunner::fireClicked()
{
	ui.mShowResults->clear();
	calculate();
	showLine(QString("%1 %2 %3 %4 %5 %6").arg("Yards",d_spc).arg("Vel",d_spc).arg("Fpe",d_spc).arg("Drop",d_spc).arg("Drift",d_spc).arg("TOF",d_spc));
	showOutput();

	// See if we need to make a csv file
	if(ui.mCreateFile->isChecked()) {
		QFile file(QString("%1.csv").arg(makeFileName()));
		if(file.open(QIODevice::WriteOnly)) {
			QTextStream stream(&file);
			stream << "Yards" << "\tVelocity" << "\tFpe" << "\tDrop" << "\tDrift" << "\tTOF" << endl;

			int yards = 0;
			double vel;
			double energy;
			double drop;

			// Begin with data every yard
			while((yards < mVelocity.count()) && (yards * data_interval) < 50) {
				vel = mVelocity.at(yards);
				energy = vel*vel*mWeight/(2.0*g_English*grains_per_pound);
				drop = mSlope*yards - (mDrop.at(yards));
				stream << QString("%1\t%2\t%3\t%4\t%5\t%6").arg(yards).arg(vel,0,'f',0).arg(energy,0,'f',1).arg(drop,0,'f',1).arg(mDrift.at(yards),0,'f',1).arg(mTof.at(yards),0,'f',3);
				stream << endl;
				++yards;
			}

			// At 50-100 yards, go up by 2
			while((yards < mVelocity.count()) && (yards * data_interval) < 100) {
				vel = mVelocity.at(yards);
				energy = vel*vel*mWeight/(2.0*g_English*grains_per_pound);
				drop = mSlope*yards - (mDrop.at(yards));
				stream << QString("%1\t%2\t%3\t%4\t%5\t%6").arg(yards).arg(vel,0,'f',0).arg(energy,0,'f',1).arg(drop,0,'f',1).arg(mDrift.at(yards),0,'f',1).arg(mTof.at(yards),0,'f',3);
				stream << endl;
				yards += 2;
			}

			// Now go up by 5 yard intervals
			while((yards < mVelocity.count()) && (yards * data_interval) < 400) {
				vel = mVelocity.at(yards);
				energy = vel*vel*mWeight/(2.0*g_English*grains_per_pound);
				drop = mSlope*yards - (mDrop.at(yards));
				stream << QString("%1\t%2\t%3\t%4\t%5\t%6").arg(yards).arg(vel,0,'f',0).arg(energy,0,'f',1).arg(drop,0,'f',1).arg(mDrift.at(yards),0,'f',1).arg(mTof.at(yards),0,'f',3);
				stream << endl;
				yards += 5;
			}

		}
	}
}
// --------------------------------------------------------
//			PRIVATE METHODS
// --------------------------------------------------------
double
Shotgunner::calcBallWeight()
{
	double rad = mShotSize/2.0;	// Get the radius
	double vol = rad*rad*rad*_pi_*4.0/3.0;
	return vol*mShotDensity*grains_per_pound;
}
// --------------------------------------------------------------------
void
Shotgunner::calcRelDensity()
{
	const double std_pres = 14.7;	// Standard air pressure at sea level

	// Calculate the air pressure at the altitude
	double alt = ui.mAltitude->itemData(ui.mAltitude->currentIndex()).toDouble();
	double pres = 14.7*pow((1 - 6.8877E-6*alt),5.25588);	// Pressure at altitude in ft.

	// Adjust pressure for non-standard temp
	const double abs_f = 459.7;	// 
	double relTP = (abs_f + lapsedTemp(alt))/(abs_f + mTemp);
	pres *= relTP;	// Pressure adjusted for temperature
	mRelDensity = pres/std_pres;
}
// --------------------------------------------------------------------
// Performs the calculation
void
Shotgunner::calculate()
{
	const int max_intervals = 1000;	//********* was 2000
	const double time_interval = 0.001;	// Integration interval
	
	double hVel = mMuzzleVelocity;	// Horizontal velocity
	double hPosn = 0;	// Horizontal distance
	double vPosn = 0;	// Vertical distance
	double xVel = 0;		// Cross-velocity
	double xPosn = 0;	// Cross-position
	double currTime = 0;	
	double bulletMass = mWeight/(grains_per_pound*g_English);
	double bulletArea = mShotSize*mShotSize*_pi_/4.0;
	mSlope = 0.0;
		
	mVelocity.clear();
	mDrop.clear();
	mDrift.clear();
	mTof.clear();

	int intervalNum = 0;
	int yardNum = 0;
	
	while(intervalNum < max_intervals) {
		// To
		currTime = intervalNum*time_interval;

		// Increment distance traveled over interval
		hPosn += hVel*time_interval;
		xPosn += xVel*time_interval;
		
		// Vertical fall due to gravity
		// s = 0.5 at^2
		vPosn = 0.5*g_English*currTime*currTime;
		
		// Find force on bullet
		// Total velocity is the vector sum of the velocity and crosswind
		double xFPS = mXWind*mph_to_fps - xVel;
		double vT = ::sqrt(hVel*hVel + xFPS*xFPS);
		double force = rValue(vT)*bulletArea*drag_coeff*mRelDensity/BC;

		// Resolve force into cross and forward vectors
		double xForce = force*xFPS/hVel;
		double hForce = force*(hVel - xFPS)/hVel;

		double hAccel = hForce/bulletMass;
		double xAccel = xForce/bulletMass;
		double deltaHV = hAccel*time_interval;
		double deltaXV = xAccel*time_interval;
		hVel -= deltaHV;
		xVel += deltaXV;

		// See if we are at a yard marker - we take data every yard...
		if((hPosn/3.0) > (double)(yardNum*data_interval)) {
			mVelocity.append(vT);
			mDrift.append(xPosn*in_per_foot);
			mDrop.append(vPosn*in_per_foot);
			mTof.append(currTime);	// TOF in seconds
			++yardNum;
		}

		++intervalNum;
	}

	double minVel = sqrt(1/(mWeight/(2.0*g_English*grains_per_pound)));
	int yards = 0;
	while(yards < mVelocity.count() && mVelocity.at(yards) > minVel) {
	    mMaxYards = yards;
	    ++yards;
	}
}
// --------------------------------------------------------------------
void
Shotgunner::doOutputLine(int yards)
{
	double vel = mVelocity.at(yards);
	if(vel > 0.0) {
		double energy = vel*vel*mWeight/(2.0*g_English*grains_per_pound);
		showLine(QString("%1 %2 %3 %4 %5 %6").arg(yards,d_spc).arg(vel,d_spc,'f',0).arg(energy,d_spc,'f',energy < 100? 1:0).arg(mSlope*yards - (mDrop.at(yards)),d_spc,'f',1).arg(mDrift.at(yards),d_spc,'f',1).arg(mTof.at(yards),d_spc,'f',3));
	}
}
// --------------------------------------------------------------------
// 'Standard' temperature at a given altitude
double
Shotgunner::lapsedTemp(double alt)
{
	return std_temp_f - temp_lapse_rate*alt;
}
// --------------------------------------------------------------------
// Make up a file name from the data
QString
Shotgunner::makeFileName()
{
	QString name = QString("c%1_w%2_v%3_x%4").arg(mShotSize,0,'f',2).arg(mWeight,0,'f',1).arg(mMuzzleVelocity,0,'f',0).arg(mXWind,0,'f',0);
	name = name.replace('.','d');	// Don't allow periods...
	return name;
}
// --------------------------------------------------------------------
// Calculates retardation as a function of velocity, based upon the 1904 British
// data.
double
Shotgunner::rValue(double vel)
{
	// British data
	const double fact_m1	 	= 74422/1.0E8;
	const double fact_m2 	= 59939/1.0E12;
	const double fact_m3 	= 23385/1.0E22;
	const double fact_m4 	= 95408/1.0E12;
	const double fact_m5 	= 59814/1.0E8;
	const double fact_m6 	= 58497/1.0E7;
	const double fact_m7 	= 15366/1.0E7;
	const double expn_1	= 1.6;
	const double expn_2	= 3.0;
	const double expn_3	= 6.45;
	const double expn_4	= 3.0;
	const double expn_5	= 1.8;
	const double expn_6	= 1.5;
	const double expn_7	= 1.67;
	
	if(vel <= 840) {
		return ::pow(vel,expn_1)*fact_m1;
	} else if(vel <= 1040) {
		return ::pow(vel,expn_2)*fact_m2;
	} else if(vel <= 1190) {
		return ::pow(vel,expn_3)*fact_m3;
	} else if(vel <= 1460) {
		return ::pow(vel,expn_4)*fact_m4;
	} else if(vel <= 2000) {
		return ::pow(vel,expn_5)*fact_m5;
	} else if(vel <= 2600) {
		return ::pow(vel,expn_6)*fact_m6;
	} else {
		return ::pow(vel,expn_7)*fact_m7;
	}
}
// --------------------------------------------------------
void
Shotgunner::showInput()
{
	ui.mEditWeight->setText(QString("%1").arg(mWeight,0,'f',mWeight < 10.0? 2:1));
	ui.mEditVelocity->setText(QString("%1").arg(mMuzzleVelocity,0,'f',0));
	ui.mEditWind->setText(QString("%1").arg(mXWind,0,'f',0));
	ui.mEditTemp->setText(QString("%1").arg(mTemp,0,'f',0));
}
// --------------------------------------------------------
void
Shotgunner::showLine(QString line)
{
	ui.mShowResults->append(line);
}
// --------------------------------------------------------
void
Shotgunner::showOutput()
{
	int yards = 0;

	// Collect data every five yards
	while((yards < mVelocity.count()) && (yards * data_interval) <= 150) {
	    if(yards < mMaxYards) {
		doOutputLine(yards);
		yards += 5;
	      } else {
		doOutputLine(mMaxYards);
		return;	// Break off if finished
	      }
	}

}
// --------------------------------------------------------


