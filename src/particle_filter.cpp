/*
 * particle_filter.cpp
 *
 *  Created on: Dec 12, 2016
 *      Author: Tiffany Huang
 */

#include <random>
#include <algorithm>
#include <iostream>
#include <numeric>
#include <math.h> 
#include <iostream>
#include <sstream>
#include <string>
#include <iterator>

#include "particle_filter.h"

using namespace std;
static default_random_engine gen;

void ParticleFilter::init(double x, double y, double theta, double std[]) {
	// Set the number of particles. Initialize all particles to first position (based on estimates of
	//   x, y, theta and their uncertainties from GPS) and all weights to 1. 
	// Add random Gaussian noise to each particle.
	// NOTE: Consult particle_filter.h for more information about this method (and others in this file).
    
    num_particles = 100;
    
    // This line creates a normal (Gaussian) distribution for x.
    normal_distribution<double> dist_x(x, std[0]);
    normal_distribution<double> dist_y(y, std[1]);
    normal_distribution<double> dist_theta(theta, std[2]);

    for (int i=0; i<num_particles; i++) {
        Particle particle;
        particle.id = i;
        particle.x = dist_x(gen);
        particle.y = dist_y(gen);
        particle.theta = dist_theta(gen);
        particle.weight = 1.0;
        particles.push_back(particle);
        weights.push_back(1.0);
    }
    is_initialized = true;
}

void ParticleFilter::prediction(double delta_t, double std_pos[], double velocity, double yaw_rate) {
	// Add measurements to each particle and add random Gaussian noise.
	// NOTE: When adding noise you may find std::normal_distribution and std::default_random_engine useful.
	//  http://en.cppreference.com/w/cpp/numeric/random/normal_distribution
	//  http://www.cplusplus.com/reference/random/default_random_engine/
    
    for (int i=0; i < particles.size(); i++) {
        Particle& p = particles[i];
        
        // Predict (x, y, theta) with delta_t, velocity and yaw_rate
        double x_f, y_f, theta_f;
        if (fabs(yaw_rate) > 0.001) {
            double yaw_angel = yaw_rate * delta_t;
            x_f = p.x + (velocity / yaw_rate) * (sin(p.theta + yaw_angel) - sin(p.theta));
            y_f = p.y + (velocity / yaw_rate) * (cos(p.theta) - cos(p.theta + yaw_angel));
            theta_f = p.theta + yaw_angel;
        } else {
            x_f = p.x + velocity * delta_t * cos(p.theta);
            y_f = p.y + velocity * delta_t * sin(p.theta);
            theta_f = p.theta;
        }
        // Add random Gaussian noise
        normal_distribution<double> dist_x(x_f, std_pos[0]);
        normal_distribution<double> dist_y(y_f, std_pos[1]);
        normal_distribution<double> dist_theta(theta_f, std_pos[2]);
        p.x = dist_x(gen);
        p.y = dist_y(gen);
        p.theta = dist_theta(gen);
    }
}

void ParticleFilter::dataAssociation(std::vector<LandmarkObs> predicted, std::vector<LandmarkObs>& observations) {
	// TODO: Find the predicted measurement that is closest to each observed measurement and assign the 
	//   observed measurement to this particular landmark.
	// NOTE: this method will NOT be called by the grading code. But you will probably find it useful to 
	//   implement this method and use it as a helper during the updateWeights phase.

}

void ParticleFilter::updateWeights(double sensor_range, double std_landmark[], 
		const std::vector<LandmarkObs> &observations, const Map &map_landmarks) {
	// Update the weights of each particle using a mult-variate Gaussian distribution. You can read
	//   more about this distribution here: https://en.wikipedia.org/wiki/Multivariate_normal_distribution
	// NOTE: The observations are given in the VEHICLE'S coordinate system. Your particles are located
	//   according to the MAP'S coordinate system. You will need to transform between the two systems.
	//   Keep in mind that this transformation requires both rotation AND translation (but no scaling).
	//   The following is a good resource for the theory:
	//   https://www.willamette.edu/~gorr/classes/GeneralGraphics/Transforms/transforms2d.htm
	//   and the following is a good resource for the actual equation to implement (look at equation 
	//   3.33
	//   http://planning.cs.uiuc.edu/node99.html
    
    for (int i = 0; i < particles.size(); i++) {
        double new_weight = 1.0;
        Particle& p = particles[i];
        for (int j = 0; j < observations.size(); j++) {
            // Transformation
            LandmarkObs obs = observations[j];
            double x_m = p.x + (cos(p.theta) * obs.x) - (sin(p.theta) * obs.y);
            double y_m = p.y + (sin(p.theta) * obs.x) + (cos(p.theta) * obs.y);
            
            // Associate observation and landmark
            Map::single_landmark_s nearestLandmark;
            double min_distance = MAXFLOAT;
            for (int k = 0; k < map_landmarks.landmark_list.size(); k++) {
                Map::single_landmark_s landmark = map_landmarks.landmark_list[k];
                double distance = pow((x_m - landmark.x_f), 2) + pow((y_m - landmark.y_f), 2);
                if (distance < min_distance) {
                    min_distance = distance;
                    nearestLandmark.x_f = landmark.x_f;
                    nearestLandmark.y_f = landmark.y_f;
                }
            }
            if (sqrt(min_distance) > sensor_range) {
                cout << "Can't associate observation with landmark within sensor range. Kill particle. min distance: " << sqrt(min_distance) << endl;
                new_weight = 0.0;
                break;
            }
            // Calculate weight
            double gauss_norm= (1 / (2 * M_PI * std_landmark[0] * std_landmark[1]));
            double exponent = pow((x_m - nearestLandmark.x_f), 2) / (2 * pow(std_landmark[0], 2));
            exponent += pow((y_m - nearestLandmark.y_f), 2) / (2 * pow(std_landmark[1], 2));
            double weight = gauss_norm * exp(-exponent);
            new_weight *= weight;
        }
        p.weight = new_weight;
        weights[i] = new_weight;
    }
}

void ParticleFilter::resample() {
	// Resample particles with replacement with probability proportional to their weight.
	// NOTE: You may find std::discrete_distribution helpful here.
	//       http://en.cppreference.com/w/cpp/numeric/random/discrete_distribution

    discrete_distribution<double> d(weights.begin(), weights.end());
    vector<Particle> resampled_particles;
    for (int i=0; i<particles.size(); i++) {
        resampled_particles.push_back(particles[d(gen)]);
    }
    particles = resampled_particles;
    for (int i=0; i<particles.size(); i++) {
        weights[i] = particles[i].weight;
    }
}

Particle ParticleFilter::SetAssociations(Particle& particle, const std::vector<int>& associations, 
                                     const std::vector<double>& sense_x, const std::vector<double>& sense_y)
{
    //particle: the particle to assign each listed association, and association's (x,y) world coordinates mapping to
    // associations: The landmark id that goes along with each listed association
    // sense_x: the associations x mapping already converted to world coordinates
    // sense_y: the associations y mapping already converted to world coordinates

    particle.associations= associations;
    particle.sense_x = sense_x;
    particle.sense_y = sense_y;
    return particle;
}

string ParticleFilter::getAssociations(Particle best)
{
	vector<int> v = best.associations;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<int>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseX(Particle best)
{
	vector<double> v = best.sense_x;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
string ParticleFilter::getSenseY(Particle best)
{
	vector<double> v = best.sense_y;
	stringstream ss;
    copy( v.begin(), v.end(), ostream_iterator<float>(ss, " "));
    string s = ss.str();
    s = s.substr(0, s.length()-1);  // get rid of the trailing space
    return s;
}
