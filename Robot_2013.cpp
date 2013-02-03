#include "WPILib.h"
#include "Vision/RGBImage.h"
#include "Vision/BinaryImage.h"
#include "Math.h"
#include "CameraCode.cpp"
#include "Turret.h"
#include "Loader.h"
#include "CoDriver.h"
#include "MainDriver.cpp"
//#include "DashBoardControl.cpp"

// Note: Currently the code regarding to dashboard is commented out line #s are: 56 154

/**
 * Sample program to use NIVision to find rectangles in the scene that are illuminated
 * by a LED ring light (similar to the model from FIRSTChoice). The camera sensitivity
 * is set very low so as to only show light sources and remove any distracting parts
 * of the image.
 * 
 * The CriteriaCollection is the set of criteria that is used to filter the set of
 * rectangles that are detected. In this example we're looking for rectangles with
 * a minimum width of 30 pixels and maximum of 400 pixels.
 * 
 * The algorithm first does a color threshold operation that only takes objects in the
 * scene that have a bright green color component. Then a convex hull operation fills 
 * all the rectangle outlines (even the partially occluded ones). Then a small object filter
 * removes small particles that might be caused by green reflection scattered from other 
 * parts of the scene. Finally all particles are scored on rectangularity, aspect ratio,
 * and hollowness to determine if they match the target.
 *
 * Look in the VisionImages directory inside the project that is created for the sample
 * images as well as the NI Vision Assistant file that contains the vision command
 * chain (open it with the Vision Assistant)
 */



class Robot_2013 : public SimpleRobot
{
	//Structure to represent the scores for the various tests used for target identification
/*	struct Scores {
		double rectangularity;
		double aspectRatioInner;
		double aspectRatioOuter;
		double xEdge;
		double yEdge;

	};*/
	
	Scores *scores;
	cameraCode *cameraFunctions;
//	dashboardControl *DBControl;

public:
	Robot_2013(void) {
		cameraFunctions = new cameraCode;
//		DBControl = new dashboardControl;

	}

	/**
	 * Image processing code to identify 2013 Vision targets
	 */
	void Autonomous(void)
	{	
		
		// instantiate lifter for aiming
		Lifter *spikeLifter;
		spikeLifter = new Lifter;
		// instantiate shooter for firing 
		Shooter *frisbeeShooter;
		frisbeeShooter = new Shooter;
		Threshold threshold(60, 100, 90, 255, 20, 255);	//HSV threshold criteria, ranges are in that order ie. Hue is 60-100
		ParticleFilterCriteria2 criteria[] = {
				{IMAQ_MT_AREA, AREA_MINIMUM, 65535, false, false}
		};												//Particle filter criteria, used to filter out small particles
//		 AxisCamera &camera = AxisCamera::GetInstance();	//To use the Axis camera uncomment this line
		printf("we are in autonomous\n");
	
		// Reset the actuator for the lifter and do a shot
		spikeLifter->lower();
		Wait(3);
		spikeLifter->off();
		frisbeeShooter->fire();

		while (IsAutonomous() && IsEnabled()) {
            /**
             * Do the image capture with the camera and apply the algorithm described above. This
             * sample will either get images from the camera or from an image file stored in the top
             * level directory in the flash memory on the cRIO. The file name in this case is "testImage.jpg"
             */
			ColorImage *image;
		
			image = new RGBImage("/testImage.jpg");		// get the sample image from the cRIO flash

//			camera.GetImage(image);				//To get the images from the camera comment the line above and uncomment this one
			BinaryImage *thresholdImage = image->ThresholdHSV(threshold);	// get just the green target pixels
			//thresholdImage->Write("/threshold.bmp");
			BinaryImage *convexHullImage = thresholdImage->ConvexHull(false);  // fill in partial and full rectangles
			//convexHullImage->Write("/ConvexHull.bmp");
			BinaryImage *filteredImage = convexHullImage->ParticleFilter(criteria, 1);	//Remove small particles
			//filteredImage->Write("Filtered.bmp");

			vector<ParticleAnalysisReport> *reports = filteredImage->GetOrderedParticleAnalysisReports();  //get a particle analysis report for each particle
			scores = new Scores[reports->size()];
			
			//Iterate through each particle, scoring it and determining whether it is a target or not
			for (unsigned i = 0; i < reports->size(); i++) {
				ParticleAnalysisReport *report = &(reports->at(i));
				
				scores[i].rectangularity = cameraFunctions->scoreRectangularity(report);
				scores[i].aspectRatioOuter = cameraFunctions->scoreAspectRatio(filteredImage, report, true);
				scores[i].aspectRatioInner = cameraFunctions->scoreAspectRatio(filteredImage, report, false);			
				scores[i].xEdge = cameraFunctions->scoreXEdge(thresholdImage, report);
				scores[i].yEdge = cameraFunctions->scoreYEdge(thresholdImage, report);
				
				if(cameraFunctions->scoreCompare(scores[i], false))
				{
					printf("particle: %d  is a High Goal  centerX: %f  centerY: %f \n", i, report->center_mass_x_normalized, report->center_mass_y_normalized);
					printf("Distance: %f \n", cameraFunctions->computeDistance(thresholdImage, report, false));
				} else if (cameraFunctions->scoreCompare(scores[i], true)) {
					printf("particle: %d  is a Middle Goal  centerX: %f  centerY: %f \n", i, report->center_mass_x_normalized, report->center_mass_y_normalized);
					printf("Distance: %f \n", cameraFunctions->computeDistance(thresholdImage, report, true));
				} else {
					printf("particle: %d  is not a goal  centerX: %f  centerY: %f \n", i, report->center_mass_x_normalized, report->center_mass_y_normalized);
				}
				printf("rect: %f  ARinner: %f \n", scores[i].rectangularity, scores[i].aspectRatioInner);
				printf("ARouter: %f  xEdge: %f  yEdge: %f  \n", scores[i].aspectRatioOuter, scores[i].xEdge, scores[i].yEdge);	
			}
			printf("\n");
			
			// be sure to delete images after using them
			delete filteredImage;
			delete convexHullImage;
			delete thresholdImage;
			delete image;
			
			//delete allocated reports and Scores objects also
			delete scores;
			delete reports;

		}
		// delete instances of other classes that we utilized
		delete spikeLifter;
		delete frisbeeShooter;
	}

	/**
	 * Runs the motors with arcade steering. 
	 */
	void OperatorControl(void)
	{
		mainDriver *Driver1;
		Driver1 = new mainDriver;
		
		// instantiate lifter for aiming
		Lifter *spikeLifter;
		spikeLifter = new Lifter;
	
		// instantiate shooter for firing 
		Shooter *frisbeeShooter;
		frisbeeShooter = new Shooter;
		Limiter *limitElevator;
		limitElevator = new Limiter;

		// Instantiate the co-driver object
		coDriver *Driver2;
		Driver2 = new coDriver;
//		DBControl->dashboardOut(1);
		printf("starting Teleop\n");
//		spikeLifter->cycle_linear_actuator(true);
		Driver1->disableSafety();
		printf("we are in teleop, accepting joystick input now\n");
		
		while (IsOperatorControl())
		{
			printf("step 1... ");
			if (limitElevator->checkLimit()) {
				Driver1->killDrive();
				printf("step 1.5\n");
			}
			else {
				Driver1->teleopDrive();
				printf("step 1.5\n");
			}
			printf("step 2... ");
			// next we do the checks to see what the codriver is trying to do
			if (Driver2->raiseCheck(spikeLifter) == false) {
				printf("step 2.1... ");
				if (Driver2->lowerCheck(spikeLifter) == false) {
					printf("step 2.2... ");
					Driver2->stopCheck(spikeLifter);
				}
			}
			printf("looping");
			Wait(0.005);				// wait for a motor update time
		}
		delete limitElevator;
		delete frisbeeShooter;
		delete spikeLifter;
		delete Driver1;
		delete Driver2;
	}

	void Disabled(void)
	{
		printf("I\'m Disabled!!\n"); // This code runs whenever the robot is disabled, even if the printf buffer sometimes forgets to flush
	}
	
};

START_ROBOT_CLASS(Robot_2013);

