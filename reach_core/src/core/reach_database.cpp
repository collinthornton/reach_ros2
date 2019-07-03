#include <reach/core/reach_database.h>
#include <reach/utils/database_utils.h>

namespace
{

reach_msgs::ReachDatabase toReachDatabase(const std::unordered_map<std::string, reach_msgs::ReachRecord>& map,
                                          const reach::core::StudyResults& results)
{
  reach_msgs::ReachDatabase msg;
  for (auto it = map.begin(); it != map.end(); ++it)
  {
    msg.records.push_back(it->second);
  }

  msg.total_pose_score = results.total_pose_score;
  msg.norm_total_pose_score = results.norm_total_pose_score;
  msg.reach_percentage = results.reach_percentage;
  msg.avg_num_neighbors = results.avg_num_neighbors;
  msg.avg_joint_distance = results.avg_joint_distance;

  return msg;
}

} // namespace anonymous

namespace reach
{
namespace core
{

void ReachDatabase::save(const std::string &filename) const
{
  std::lock_guard<std::mutex> lock {mutex_};
  reach_msgs::ReachDatabase msg = toReachDatabase(map_, results_);

  if (!reach::utils::toFile(filename, msg))
  {
    throw std::runtime_error("Unable to save database to file: " + filename);
  }
}

bool ReachDatabase::load(const std::string &filename)
{
  reach_msgs::ReachDatabase msg;
  if (!reach::utils::fromFile(filename, msg))
  {
    return false;
  }

  std::lock_guard<std::mutex> lock {mutex_};

  for (const auto& r : msg.records)
  {
    putHelper(r);
    results_.reach_percentage = msg.reach_percentage;
    results_.total_pose_score = msg.total_pose_score;
    results_.norm_total_pose_score = msg.norm_total_pose_score;
    results_.avg_num_neighbors = msg.avg_num_neighbors;
    results_.avg_joint_distance = msg.avg_joint_distance;
  }
  return true;
}

boost::optional<reach_msgs::ReachRecord> ReachDatabase::get(const std::string &id) const
{
  std::lock_guard<std::mutex> lock {mutex_};
  auto it = map_.find(id);
  if (it != map_.end())
  {
    return {it->second};
  }
  else
  {
    return {};
  }
}

void ReachDatabase::put(const reach_msgs::ReachRecord &record)
{
  std::lock_guard<std::mutex> lock {mutex_};
  return putHelper(record);
}

void ReachDatabase::putHelper(const reach_msgs::ReachRecord &record)
{
  map_[record.id] = record;
}

std::size_t ReachDatabase::size() const
{
  return map_.size();
}

void ReachDatabase::calculateResults()
{
  unsigned int success = 0, total = 0;
  double score = 0.0;
  for(int i = 0; i < this->size(); ++i)
  {
    reach_msgs::ReachRecord msg = *this->get(std::to_string(i));

    if(msg.reached)
    {
      success++;
      score += msg.score;
    }

    total ++;
  }
  const float pct_success = static_cast<float>(success) / static_cast<float>(total);

  results_.reach_percentage = 100.0f * pct_success;
  results_.total_pose_score = score;
  results_.norm_total_pose_score = score / pct_success;
}

void ReachDatabase::printResults()
{
  ROS_INFO("------------------------------------------------");
  ROS_INFO_STREAM("Percent Reached = " << results_.reach_percentage);
  ROS_INFO_STREAM("Total points score = " << results_.total_pose_score);
  ROS_INFO_STREAM("Normalized total points score = " << results_.norm_total_pose_score);
  ROS_INFO_STREAM("Average reachable neighbors = " << results_.avg_num_neighbors);
  ROS_INFO_STREAM("Average joint distance = " << results_.avg_joint_distance);
  ROS_INFO_STREAM("------------------------------------------------");
}

reach_msgs::ReachDatabase ReachDatabase::toReachDatabaseMsg()
{
  return toReachDatabase(map_, results_);
}

} // namespace core
} // namespace reach

