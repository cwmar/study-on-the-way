#pragma once
#include <stdio.h>

#include <chrono>
#include <examples/examples.hpp>
#include <thread>
#include <xdds.hpp>

namespace xdds {
namespace examples {

class TalkerNode {
 public:
  using Chatter = xpilot_dds::examples::Chatter;
  using Driver = xpilot_dds::examples::Driver;
  TalkerNode(uint32_t domain_id = 80);
  ~TalkerNode();
  bool InitPublisher();
  bool Publish();

  void HighFreqThread();
  void LowFreqThread();

 private:
  uint32_t domain_id_;
  std::shared_ptr<xdds::domain::DomainParticipant> participant_{};
  std::shared_ptr<xdds::pub::Publisher> publisher_a_{};
  std::shared_ptr<xdds::pub::Publisher> publisher_b_{};
  std::shared_ptr<xdds::pub::DataWriter<Chatter>> high_chatter_writer_{};
  std::shared_ptr<xdds::pub::DataWriter<Driver>> high_driver_writer_{};
  std::shared_ptr<xdds::pub::DataWriter<Chatter>> low_chatter_writer_{};
  std::shared_ptr<xdds::pub::DataWriter<Driver>> low_driver_writer_{};
};

}  // namespace examples
}  // namespace xdds