#include "talker_class.hpp"

namespace xdds {
namespace examples {

using DomainParticipantFactory = xdds::domain::DomainParticipantFactory;
const char* HIGH_CHATTER = "HighChatter";
const char* LOW_CHATTER = "LowChatter";
const char* HIGH_DRIVER = "HighDriver";
const char* LOW_DRIVER = "LowDriver";

const char* PUBA = "PublisherA";
const char* PUBB = "PublisherB";

class CustomDataWriterListener : public pub::DataWriterListener {
 public:
  void OnPublicationMatched(
      pub::BaseDataWriter* writer,
      const core::status::PublicationMatchedStatus& info) override {
    topic_name_ = writer->GetTopic()->GetName();
    std::cout << "Writer Topic: " << topic_name_ << " got a match publication." << std::endl;
  }
  void OnLivelinessLost(
      pub::BaseDataWriter* writer,
      const core::status::LivelinessLostStatus& status) override {}
  std::string GetMatchedTopicName() const { return topic_name_; }

 private:
  std::string topic_name_;
};

TalkerNode::TalkerNode(uint32_t domain_id) : domain_id_(domain_id) {
  participant_ =
      DomainParticipantFactory::Instance()->CreateParticipant(domain_id_);
}

TalkerNode::~TalkerNode() {
  if (participant_ != nullptr) {
    DomainParticipantFactory::Instance()->DeleteParticipant(participant_);
  }
  participant_ = nullptr;
  publisher_a_ = nullptr;
  publisher_b_ = nullptr;
  high_chatter_writer_ = nullptr;
  high_driver_writer_ = nullptr;
  low_chatter_writer_ = nullptr;
  low_driver_writer_ = nullptr;
}

bool TalkerNode::InitPublisher() {
  if (participant_ == nullptr) {
    std::cout << "Participant is not inited." << std::endl;
    return false;
  }

  publisher_a_ = participant_->CreatePublisher(PUBA);
  if (publisher_a_->Enable() != ReturnCode_t::RETCODE_OK) {
    fprintf(stderr, "Failed to Enable highfreq_publisher_");
    return false;
  }
  publisher_b_ = participant_->CreatePublisher(PUBB);
  if (publisher_b_->Enable() != ReturnCode_t::RETCODE_OK) {
    fprintf(stderr, "Failed to Enable lowfreq_publisher_");
    return false;
  }
  auto topic_high_chatter = participant_->CreateTopic(HIGH_CHATTER);
  high_chatter_writer_ =
      publisher_a_->CreateDataWriter<Chatter>(topic_high_chatter);
  if (ReturnCode_t::RETCODE_OK != high_chatter_writer_->Enable()) {
    fprintf(stderr, "Failed to Enable high_chatter_writer_");
    return false;
  }
  high_chatter_writer_->SetListener(std::make_shared<CustomDataWriterListener>());

  auto topic_low_driver = participant_->CreateTopic(LOW_DRIVER);
  low_driver_writer_ =
      publisher_a_->CreateDataWriter<Driver>(topic_low_driver);
  if (ReturnCode_t::RETCODE_OK != low_driver_writer_->Enable()) {
    fprintf(stderr, "Failed to Enable low_driver_writer_");
    return false;
  }
  low_driver_writer_->SetListener(std::make_shared<CustomDataWriterListener>());

  auto topic_low_chatter = participant_->CreateTopic(LOW_CHATTER);
  low_chatter_writer_ =
      publisher_b_->CreateDataWriter<Chatter>(topic_low_chatter);
  if (ReturnCode_t::RETCODE_OK != low_chatter_writer_->Enable()) {
    fprintf(stderr, "Failed to Enable low_chatter_writer_");
    return false;
  }
  low_chatter_writer_->SetListener(std::make_shared<CustomDataWriterListener>());

  auto topic_high_driver = participant_->CreateTopic(HIGH_DRIVER);
  high_driver_writer_ =
      publisher_b_->CreateDataWriter<Driver>(topic_high_driver);
  if (ReturnCode_t::RETCODE_OK != high_driver_writer_->Enable()) {
    fprintf(stderr, "Failed to Enable high_driver_writer_");
    return false;
  }
  high_driver_writer_->SetListener(std::make_shared<CustomDataWriterListener>());
  return true;
}

void TalkerNode::HighFreqThread() {
  std::shared_ptr<Chatter> chatter(new Chatter());
  std::shared_ptr<Driver> driver(new Driver());
  uint64_t index = 0;
  while (true) {
    chatter->content("HighChatter: " + std::to_string(index));
    driver->content("HighDriver: " + std::to_string(index));
    chatter->seq(index);
    driver->msg_id(index);
    high_chatter_writer_->Write(chatter);
    high_driver_writer_->Write(driver);
    index++;
    if (index % 100 == 0)
      std::cout << "High: Sending index " << std::to_string(index) << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

void TalkerNode::LowFreqThread() {
  auto chatter = std::make_shared<Chatter>();
  auto driver = std::make_shared<Driver>();

  uint64_t index = 0;
  while (true) {
    chatter->content("LowChatter: " + std::to_string(index));
    driver->content("LowDriver: " + std::to_string(index));
    chatter->seq(index);
    driver->msg_id(index);
    low_chatter_writer_->Write(chatter);
    low_driver_writer_->Write(driver);
    index++;
    if (index % 10 == 0)
      std::cout << "Low: Sending index " << std::to_string(index) << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }
}

bool TalkerNode::Publish() {
  auto fast = std::thread(&TalkerNode::HighFreqThread, this);
  auto low = std::thread(&TalkerNode::LowFreqThread, this);
  fast.join();
  low.join();
  return true;
}

}  // namespace examples
}  // namespace xdds

namespace {
const uint32_t ORIN_A_DOMAIN_ID = 80;
}

int main() {
  xdds::Init();  // add after main
  auto node = std::make_shared<xdds::examples::TalkerNode>(ORIN_A_DOMAIN_ID);
  node->InitPublisher();
  node->Publish();
  xdds::Shutdown();
  return 0;
}