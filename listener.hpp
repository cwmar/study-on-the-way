#pragma once
#include <stdio.h>

#include <chrono>
#include <examples/examples.hpp>
#include <thread>
#include <xdds.hpp>

namespace xdds {
namespace examples {

class ListenerNode {
 public:
  using Chatter = xpilot_dds::examples::Chatter;
  using Driver = xpilot_dds::examples::Driver;
  ListenerNode(uint32_t domain_id = 80);
  ~ListenerNode();
  bool InitSubscriber();
  bool Subscribe();

  void WaitsetThread();
  void LowDriverDataProcess();

 private:
  uint32_t domain_id_;
  std::shared_ptr<xdds::domain::DomainParticipant> participant_{};
  std::shared_ptr<xdds::sub::Subscriber> subscriber_a_{};
  std::shared_ptr<xdds::sub::Subscriber> subscriber_b_{};
  std::shared_ptr<xdds::sub::DataReader<Chatter>> high_chatter_reader_{};
  std::shared_ptr<xdds::sub::DataReader<Driver>> high_driver_reader_{};
  std::shared_ptr<xdds::sub::DataReader<Chatter>> low_chatter_reader_{};
  std::shared_ptr<xdds::sub::DataReader<Driver>> low_driver_reader_{};
};

}  // namespace examples
}  // namespace xdds