#pragma once

#include <units/time.h>
#include <units/length.h>
#include <units/velocity.h>
#include <units/acceleration.h>
#include <units/angular_velocity.h>
#include <units/curvature.h>
#include <frc/geometry/Translation2d.h>
#include <frc/geometry/Rotation2d.h>
#include <frc/geometry/Pose2d.h>
#include <frc/kinematics/ChassisSpeeds.h>
#include <frc/kinematics/SwerveModuleState.h>
#include <frc/MathUtil.h>
#include <frc2/command/Command.h>
#include <vector>
#include <utility>
#include <memory>
#include "pathplanner/lib/trajectory/PathPlannerTrajectoryState.h"
#include "pathplanner/lib/path/PathConstraints.h"
#include "pathplanner/lib/util/GeometryUtil.h"
#include "pathplanner/lib/config/RobotConfig.h"

namespace pathplanner {

class PathPlannerPath;

class PathPlannerTrajectory {
public:
	PathPlannerTrajectory() {
	}

	/**
	 * Create a trajectory with pre-generated states and list of events
	 *
	 * @param states Pre-generated states
	 * @param eventCommands Event commands
	 */
	PathPlannerTrajectory(std::vector<PathPlannerTrajectoryState> states,
			std::vector<
					std::pair<units::second_t, std::shared_ptr<frc2::Command>>> eventCommands) : m_states(
			states), m_eventCommands(eventCommands) {
	}

	/**
	 * Create a trajectory with pre-generated states
	 *
	 * @param states Pre-generated states
	 */
	PathPlannerTrajectory(std::vector<PathPlannerTrajectoryState> states) : m_states(
			states) {
	}

	/**
	 * Generate a new trajectory for a given path
	 *
	 * @param path The path to generate a trajectory for
	 * @param startingSpeeds The starting robot-relative chassis speeds of the robot
	 * @param startingRotation The starting field-relative rotation of the robot
	 * @param config The RobotConfig describing the robot
	 */
	PathPlannerTrajectory(std::shared_ptr<PathPlannerPath> path,
			const frc::ChassisSpeeds &startingSpeeds,
			const frc::Rotation2d &startingRotation, const RobotConfig &config);

	/**
	 * Get all of the pairs of timestamps + commands to run at those timestamps
	 *
	 * @return Pairs of timestamps and event commands
	 */
	inline std::vector<
			std::pair<units::second_t, std::shared_ptr<frc2::Command>>> getEventCommands() {
		return m_eventCommands;
	}

	/**
	 * Get all of the pre-generated states in the trajectory
	 *
	 * @return vector of all states
	 */
	constexpr std::vector<PathPlannerTrajectoryState>& getStates() {
		return m_states;
	}

	/**
	 * Get the goal state at the given index
	 *
	 * @param index Index of the state to get
	 * @return The state at the given index
	 */
	inline PathPlannerTrajectoryState getState(size_t index) {
		return m_states[index];
	}

	/**
	 * Get the initial state of the trajectory
	 *
	 * @return The initial state
	 */
	inline PathPlannerTrajectoryState getInitialState() {
		return m_states[0];
	}

	/**
	 * Get the end state of the trajectory
	 *
	 * @return The end state
	 */
	inline PathPlannerTrajectoryState getEndState() {
		return m_states[m_states.size() - 1];
	}

	/**
	 * Get the total run time of the trajectory
	 *
	 * @return Total run time in seconds
	 */
	inline units::second_t getTotalTime() {
		return getEndState().time;
	}

	/**
	 * Get the initial robot pose at the start of the trajectory
	 *
	 * @return Pose of the robot at the initial state
	 */
	inline frc::Pose2d getInitialPose() {
		return getInitialState().pose;
	}

	/**
	 * Get the target state at the given point in time along the trajectory
	 *
	 * @param time The time to sample the trajectory at in seconds
	 * @return The target state
	 */
	PathPlannerTrajectoryState sample(const units::second_t time);
private:
	std::vector<PathPlannerTrajectoryState> m_states;
	std::vector<std::pair<units::second_t, std::shared_ptr<frc2::Command>>> m_eventCommands;

	static void generateStates(std::vector<PathPlannerTrajectoryState> &states,
			std::shared_ptr<PathPlannerPath> path,
			const frc::Rotation2d &startingRotation, const RobotConfig &config);

	static void forwardAccelPass(
			std::vector<PathPlannerTrajectoryState> &states,
			const RobotConfig &config);

	static void reverseAccelPass(
			std::vector<PathPlannerTrajectoryState> &states,
			const RobotConfig &config);

	static void desaturateWheelSpeeds(
			std::vector<SwerveModuleTrajectoryState> &moduleStates,
			const frc::ChassisSpeeds &desiredSpeeds,
			units::meters_per_second_t maxModuleSpeed,
			units::meters_per_second_t maxTranslationSpeed,
			units::radians_per_second_t maxRotationSpeed);

	static size_t getNextRotationTargetIdx(
			std::shared_ptr<PathPlannerPath> path, const size_t startingIndex);

	static inline frc::Rotation2d cosineInterpolate(const frc::Rotation2d start,
			const frc::Rotation2d end, const double t) {
		double t2 = (1.0 - std::cos(t * M_PI)) / 2.0;
		return GeometryUtil::rotationLerp(start, end, t2);
	}

	static inline std::vector<frc::SwerveModuleState> toSwerveModuleStates(
			RobotConfig config, frc::ChassisSpeeds chassisSpeeds) {
		if (config.isHolonomic) {
			auto states = config.swerveKinematics.ToSwerveModuleStates(
					chassisSpeeds);
			return std::vector < frc::SwerveModuleState
					> (states.begin(), states.end());
		} else {
			auto states = config.diffKinematics.ToSwerveModuleStates(
					chassisSpeeds);
			return std::vector < frc::SwerveModuleState
					> (states.begin(), states.end());
		}
	}

	static inline frc::ChassisSpeeds toChassisSpeeds(RobotConfig config,
			std::vector<SwerveModuleTrajectoryState> states) {
		if (config.isHolonomic) {
			wpi::array < frc::SwerveModuleState, 4
					> wpiStates { frc::SwerveModuleState { states[0].speed,
							states[0].angle }, frc::SwerveModuleState {
							states[1].speed, states[1].angle },
							frc::SwerveModuleState { states[2].speed,
									states[2].angle }, frc::SwerveModuleState {
									states[3].speed, states[3].angle } };
			return config.swerveKinematics.ToChassisSpeeds(wpiStates);
		} else {
			wpi::array < frc::SwerveModuleState, 2
					> wpiStates { frc::SwerveModuleState { states[0].speed,
							states[0].angle }, frc::SwerveModuleState {
							states[1].speed, states[1].angle } };
			return config.diffKinematics.ToChassisSpeeds(wpiStates);
		}
	}
};
}