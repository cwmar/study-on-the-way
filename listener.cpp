#include "listener_class.hpp"

namespace xdds {
namespace examples {
using DomainParticipantFactory = xdds::domain::DomainParticipantFactory;
using Chatter = xpilot_dds::examples::Chatter;
using Driver = xpilot_dds::examples::Driver;
using MessageInfo = xdds::transmission::transport::MessageInfo;

const char* HIGH_CHATTER = "HighChatter";
const char* LOW_CHATTER = "LowChatter";
const char* HIGH_DRIVER = "HighDriver";
const char* LOW_DRIVER = "LowDriver";
const char* SUBA = "SubscriberA";
const char* SUBB = "SubscriberB";

// Use this Listener class to receive Data in async mode.
class LowChatterListener : public sub::DataReaderListener {
 public:
  bool OnRawDataAvailable(sub::BaseDataReader* reader, const uint8_t* data,
                          const uint64_t size, const MessageInfo& msg_info) {
    // We need to return false or the further step will not be processed.
    return false;
  }
  void OnDataAvailable(sub::BaseDataReader* reader) {
    auto data_reader = dynamic_cast<sub::DataReader<Chatter>*>(reader);
    std::vector<std::shared_ptr<Chatter>> data_vec;
    std::vector<std::shared_ptr<MessageInfo>> info_vec;
    auto ret = data_reader->Take(data_vec, info_vec);
    if (ret != false) {
      std::cout << "LowChatter:Taking " << data_vec.size() << " data." << std::endl;
    }
  }
  void OnSubscriptionMatched(
      sub::BaseDataReader* reader,
      const core::status::SubscriptionMatchedStatus& info) {
    auto topic_name = reader->GetTopic()->GetName();
    std::cout << "Reader Topic: " << topic_name << " got a match subscription."
              << std::endl;
  }
};

ListenerNode::ListenerNode(uint32_t domain_id) : domain_id_(domain_id) {
  participant_ =
      DomainParticipantFactory::Instance()->CreateParticipant(domain_id_);
}

ListenerNode::~ListenerNode() {
  if (participant_ != nullptr) {
    DomainParticipantFactory::Instance()->DeleteParticipant(participant_);
  }
  participant_ = nullptr;
  subscriber_a_ = nullptr;
  subscriber_b_ = nullptr;
  high_chatter_reader_ = nullptr;
  high_driver_reader_ = nullptr;
  low_chatter_reader_ = nullptr;
  low_driver_reader_ = nullptr;
}

// Do the initilization.
bool ListenerNode::InitSubscriber() {
  if (participant_ == nullptr) {
    std::cout << "Participant is not inited." << std::endl;
    return false;
  }

  subscriber_a_ = participant_->CreateSubscriber(SUBA);
  if (subscriber_a_->Enable() != ReturnCode_t::RETCODE_OK) {
    fprintf(stderr, "Failed to Enable subscriber_a_");
    return false;
  }
  subscriber_b_ = participant_->CreateSubscriber(SUBB);
  if (subscriber_b_->Enable() != ReturnCode_t::RETCODE_OK) {
    fprintf(stderr, "Failed to Enable subscriber_b_");
    return false;
  }

  auto topic_high_chatter = participant_->CreateTopic(HIGH_CHATTER);
  high_chatter_reader_ =
      subscriber_a_->CreateDataReader<Chatter>(topic_high_chatter);
  if (ReturnCode_t::RETCODE_OK != high_chatter_reader_->Enable()) {
    fprintf(stderr, "Failed to Enable high_chatter_reader_");
    return false;
  }
  
  auto topic_low_driver = participant_->CreateTopic(LOW_DRIVER);
  low_driver_reader_ =
      subscriber_a_->CreateDataReader<Driver>(topic_low_driver);
  if (ReturnCode_t::RETCODE_OK != low_driver_reader_->Enable()) {
    fprintf(stderr, "Failed to Enable low_driver_reader_");
    return false;
  }

  auto topic_low_chatter = participant_->CreateTopic(LOW_CHATTER);
  low_chatter_reader_ =
      subscriber_b_->CreateDataReader<Chatter>(topic_low_chatter);
  if (ReturnCode_t::RETCODE_OK != low_chatter_reader_->Enable()) {
    fprintf(stderr, "Failed to Enable low_chatter_reader_");
    return false;
  }
  // Low chatter reader will take data in async listener callback.
  low_chatter_reader_->SetListener(
      std::make_shared<LowChatterListener>());

  auto topic_high_driver = participant_->CreateTopic(HIGH_DRIVER);
  high_driver_reader_ =
      subscriber_b_->CreateDataReader<Driver>(topic_high_driver);
  if (ReturnCode_t::RETCODE_OK != high_driver_reader_->Enable()) {
    fprintf(stderr, "Failed to Enable high_driver_reader_");
    return false;
  }
  return true;
}

// ReadCondition Handler for Low driver data reader. It is a sync mode to read data.
void ListenerNode::LowDriverDataProcess() {
  std::vector<std::shared_ptr<Driver>> data_vec;
  std::vector<std::shared_ptr<MessageInfo>> info_vec;
  auto ret = low_driver_reader_->Take(data_vec, info_vec);
  if (ret != false) {
    std::cout << "LowDriver:Taking " << data_vec.size() << " data." << std::endl;
    for (int i = 0; i < data_vec.size(); i++) {
      std::cout << "data_vec[" << std::to_string(i) << "]:souce:"
                << std::to_string((int)info_vec[i]->source())
                << ":content:" << data_vec[i]->content() << std::endl;
    }
  }
}

void ListenerNode::WaitsetThread() {
  // Use the embedded status condition in the reader direclty.
  xdds::core::cond::StatusCondition& cond = high_chatter_reader_->GetStatusCondition();
  cond.SetEnabledStatuses(xdds::core::status::StatusMask::DataAvailable());
  cond.SetHandler([&]() {
    std::vector<std::shared_ptr<Chatter>> data_vec;
    std::vector<std::shared_ptr<MessageInfo>> info_vec;
    auto ret = high_chatter_reader_->Take(data_vec, info_vec);
    if (ret != false) {
      std::cout << "HighChatter:Taking " << data_vec.size() << " data." << std::endl;
    }
  });

  auto readcond =
    xdds::sub::cond::ReadCondition(low_driver_reader_, xdds::sub::status::DataState::NewData(), std::bind(&ListenerNode::LowDriverDataProcess, this));
  xdds::core::cond::WaitSet wait_set;
  wait_set.AttachCondition(cond);
  wait_set.AttachCondition(readcond);
  while (true) {
    wait_set.Dispatch(common::Duration::FromSeconds(5));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

bool ListenerNode::Subscribe() {
  auto waitset = std::thread(&ListenerNode::WaitsetThread, this);
  waitset.join();
  return true;
}


}  // namespace examples
}  // namespace xdds

namespace {
const uint32_t ORIN_A_DOMAIN_ID = 80;
}

int main() {
  xdds::Init();  // add after main
  auto node = std::make_shared<xdds::examples::ListenerNode>(ORIN_A_DOMAIN_ID);
  node->InitSubscriber();
  node->Subscribe();
  xdds::Shutdown();
  return 0;
}