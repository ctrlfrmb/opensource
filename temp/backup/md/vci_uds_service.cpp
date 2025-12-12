#include "uds/vci_uds_service.h"
#include "uds/vci_uds_communicator.h"
#include "uds/vci_uds_security_provider.h"
#include "uds/vci_uds_transaction.h"
#include "uds/vci_uds_functional_transaction.h"
#include "uds/vci_uds_tp_utils.h"
#include "uds/vci_uds_utils.h"
#include "uds/vci_uds_def.h"
#include <common_api/utils.h>
#include <common_api/logger_macros.h>
#include <iterator>

namespace vci {
namespace uds {

Service::Service(int device_index, int channel_index, uint32_t request_id, uint32_t response_id) {
    Utils::initializeContext(m_context);
    m_context.device_index = device_index;
    m_context.channel_index = channel_index;
    m_context.request_id = request_id;
    m_context.response_id = response_id;
    initialize();
}

Service::Service(const UdsSessionContext& context) : m_context(context) {
    initialize();
    LOG_INFO("[VCI-UDS-SVC] {}", Utils::formatContext(m_context));
}

Service::~Service() {
    if (m_service_is_running.exchange(false, std::memory_order_acq_rel)) {
        //  立即通知所有活动停止
        abort();

        stopKeepAlive();
        stopPhysicalProcessingThread();
        stopFunctionalProcessingThread();

		// TODO DEL
        // 在销毁任何成员之前，获取事务锁。
        // 如果工作线程正在执行 executeTransaction，它会持有这个锁。
        // 那么这里的析构函数就会阻塞，直到工作线程完成并释放锁。
        // 这就保证了我们不会在工作线程使用它时销毁它。
        std::lock_guard<std::mutex> final_guard(m_transaction_mutex);

        if (m_communicator) {
            m_communicator->closeLog();
            m_communicator->shutdown();
        }
    }
    LOG_INFO("[VCI-UDS-SVC] UDS Service destroyed.");
}


void Service::initialize() {
    if (!Utils::validateContext(m_context)) {
        throw std::invalid_argument("[VCI-UDS-SVC] Context validation failed");
    }

    // if (tp::ProtocolUtils::sanitizeContext(m_context)) {
    //     LOG_WARN("[VCI-UDS-SVC] Corrected context values (e.g., st_min) for stability.");
    // }

    try {
        m_communicator = std::make_unique<Communicator>(m_context);
    } catch (const std::exception& e) {
        LOG_ERROR("[VCI-UDS-SVC] Failed to initialize Communicator: {}", e.what());
        throw;
    }
    m_security_config = {}; // Zero-initialize the security config

    m_service_is_running.store(true, std::memory_order_release);
    LOG_INFO("[VCI-UDS-SVC] UDS Service initialized for ReqID {:#x} / ResID {:#x} on Dev{}/Chn{}.",
             m_context.request_id, m_context.response_id, m_context.device_index, m_context.channel_index);
}

// --- Configuration Methods ---

int Service::setUdsConfig(const char* commands) {
    std::unique_lock<std::shared_mutex> lock(m_config_mutex);
    UdsSessionContext temp_context = m_context;

    // 解析返回值处理： <0 错误, 0 无命中(忽略), >0 命中
    int parse_result = Utils::parseUdsConfig(commands, temp_context);
    if (parse_result < 0) {
        return VCI_UDS_RESULT_CONFIG_FAILED;
    }
    if (parse_result == 0) {
        // 没有相关的配置项，直接返回成功，保持现状
        return VCI_UDS_RESULT_OK;
    }

    if (!Utils::validateContext(temp_context)) {
        return VCI_UDS_RESULT_CONFIG_FAILED;
    }

    m_context = temp_context;
    LOG_INFO("[VCI-UDS-SVC] {}", Utils::formatContext(m_context));
    return VCI_UDS_RESULT_OK;
}

int Service::setLogConfig(const char* commands) {
    UdsLogConfig temp_log_config = {}; // Zero-initialize

    int parse_result = Utils::parseLogConfig(commands, temp_log_config);
    if (parse_result < 0) {
        return VCI_UDS_RESULT_CONFIG_FAILED;
    }
    if (parse_result == 0) {
        return VCI_UDS_RESULT_OK;
    }

    if (temp_log_config.use_log) {
        if (temp_log_config.is_enable) {
            if (m_communicator->openLog(temp_log_config) != 0) {
                return VCI_UDS_RESULT_CONFIG_LOGGER_FAILED;
            }
        } else {
            m_communicator->closeLog();
        }
    }

    return VCI_UDS_RESULT_OK;
}

int Service::setSecurityConfig(const char* commands) {
    UdsSecurityAlgoConfig temp_config = {}; // Zero-initialize

    int parse_result = Utils::parseSecurityConfig(commands, temp_config);
    if (parse_result < 0) {
        return VCI_UDS_RESULT_SECURITY_CONFIG_FAILED;
    }
    if (parse_result == 0) {
        return VCI_UDS_RESULT_OK;
    }

    m_security_config = temp_config;
    LOG_INFO("[VCI-UDS-SVC] {}", Utils::formatSecurityConfig(m_security_config));
    return VCI_UDS_RESULT_OK;
}

UdsSessionContext Service::getContext() const {
    std::shared_lock<std::shared_mutex> lock(m_config_mutex);
    return m_context;
}

// --- High-Level Business APIs ---
int Service::securityAccess(uint8_t security_level, std::vector<uint8_t>& final_response) {
    return securityAccess(m_security_config, security_level, final_response);
}

int Service::securityAccess(const UdsSecurityAlgoConfig& config, uint8_t security_level, std::vector<uint8_t>& final_response) {
    std::unique_ptr<SecurityProvider> provider;
    try {
        provider = std::make_unique<SecurityProvider>(config);
    } catch (const std::invalid_argument& e) {
        LOG_ERROR("[VCI-UDS-SVC] Failed to create dynamic security provider: {}", e.what());
        return VCI_UDS_RESULT_CONFIG_FAILED;
    }
    return executeSecurityAccessSequence(*provider, security_level, final_response);
}

int Service::executeSecurityAccessSequence(SecurityProvider& provider, uint8_t security_level, std::vector<uint8_t>& final_response) {
    // This method orchestrates the sequence, but the actual send/receive is
    // protected by m_transaction_mutex inside executeTransaction.

    // --- Step 1: Request Seed ---
    uint8_t request_seed_subfunc = 2 * security_level - 1;
    std::vector<uint8_t> seed_request_payload = { 0x27, request_seed_subfunc };
    std::vector<uint8_t> seed_response_payload;
    LOG_INFO("[VCI-UDS-SVC] SecLvl {}: Requesting seed (SubFunc: {:#04x})...", (int)security_level, request_seed_subfunc);
    int ret = requestSync(seed_request_payload, seed_response_payload);
    if (ret != VCI_UDS_RESULT_OK) {
        final_response = seed_response_payload;
        LOG_ERROR("[VCI-UDS-SVC] SecLvl {}: Failed to get seed. Error: {}", (int)security_level, ret);
        return ret;
    }
    if (seed_response_payload.size() < 3 || seed_response_payload[0] != 0x67) {
        LOG_ERROR("[VCI-UDS-SVC] SecLvl {}: Invalid seed response received.", (int)security_level);
        return VCI_UDS_RESULT_SECURITY_INVALID_SEED;
    }
    std::vector<uint8_t> seed(seed_response_payload.begin() + 2, seed_response_payload.end());
    if (seed.empty()) {
        LOG_ERROR("[VCI-UDS-SVC] SecLvl {}: Seed response contains no seed data.", (int)security_level);
        return VCI_UDS_RESULT_SECURITY_INVALID_SEED;
    }

    // --- Step 2: Calculate Key ---
    LOG_INFO("[VCI-UDS-SVC] SecLvl {}: Calculating key with {}-byte seed...", (int)security_level, seed.size());
    std::vector<uint8_t> key;
    ret = provider.calculateKey(security_level, seed, key);
    if (ret != VCI_UDS_RESULT_OK) {
        LOG_ERROR("[VCI-UDS-SVC] SecLvl {}: Key calculation failed. Error: {}", (int)security_level, ret);
        return ret;
    }
    LOG_INFO("[VCI-UDS-SVC] SecLvl {}: Key calculation successful ({} bytes).", (int)security_level, key.size());

    // --- Step 3: Send Key ---
    uint8_t send_key_subfunc = 2 * security_level;
    std::vector<uint8_t> key_request_payload = { 0x27, send_key_subfunc };
    key_request_payload.insert(key_request_payload.end(), key.begin(), key.end());
    LOG_INFO("[VCI-UDS-SVC] SecLvl {}: Sending key (SubFunc: {:#04x})...", (int)security_level, send_key_subfunc);
    ret = requestSync(key_request_payload, final_response);
    if (ret == VCI_UDS_RESULT_OK) {
        LOG_INFO("[VCI-UDS-SVC] Security Access sequence completed successfully.");
    } else {
        LOG_ERROR("[VCI-UDS-SVC] Security Access sequence failed on sending key. Error: {}", ret);
    }
    return ret;
}

// --- Low-Level Communication Primitives ---
int Service::requestSync(const std::vector<uint8_t>& request_payload, std::vector<uint8_t>& response_payload) {
    Response res = executeTransaction(request_payload);
    response_payload = std::move(res.payload);
    return res.result_code;
}

int Service::requestAsync(const std::vector<uint8_t>& request_payload) {
    if (!m_service_is_running.load(std::memory_order_acquire)) return VCI_UDS_RESULT_INTERNAL_ERROR;
    if (request_payload.empty()) return VCI_UDS_RESULT_INVALID_PARAM;
    if (m_request_queue.size_approx() > REQUEST_QUEUE_SIZE) return VCI_UDS_RESULT_QUEUE_FULL;

    // On-demand start of the processing thread
    startPhysicalProcessingThread();

    m_request_queue.enqueue({request_payload});
    return VCI_UDS_RESULT_OK;
}

int Service::requestFunctional(const std::vector<uint8_t>& request_payload) {
    if (!m_service_is_running.load(std::memory_order_acquire)) return VCI_UDS_RESULT_INTERNAL_ERROR;
    if (request_payload.empty()) return VCI_UDS_RESULT_INVALID_PARAM;

    // On-demand start of the processing thread
    startFunctionalProcessingThread();

    m_functional_request_queue.enqueue({request_payload});
    return VCI_UDS_RESULT_OK;
}

// --- Thread & General Operations ---

int Service::startPhysicalProcessingThread() {
    // Fast path check without lock
    if (m_phys_thread_active.load(std::memory_order_acquire)) {
        return VCI_UDS_RESULT_OK;
    }

    std::lock_guard<std::mutex> lock(m_phys_thread_mutex);
    // Double-check after acquiring the lock
    if (m_phys_thread_active.load(std::memory_order_acquire)) {
        return VCI_UDS_RESULT_OK;
    }

    if (m_phys_processing_thread.joinable()) {
        m_phys_processing_thread.join(); // Clean up previous instance
    }
    m_phys_thread_active.store(true, std::memory_order_release);
    m_phys_processing_thread = std::thread(&Service::physicalProcessingThreadFunc, this);
    return VCI_UDS_RESULT_OK;
}

int Service::stopPhysicalProcessingThread() {
    if (!m_phys_thread_active.exchange(false, std::memory_order_acq_rel)) {
        return VCI_UDS_RESULT_OK; // Already stopped
    }
    m_request_queue.enqueue({}); // Unblock the waiting thread

    std::lock_guard<std::mutex> lock(m_phys_thread_mutex);
    if (m_phys_processing_thread.joinable()) {
        m_phys_processing_thread.join();
    }
    return VCI_UDS_RESULT_OK;
}

int Service::startFunctionalProcessingThread() {
    // Fast path check without lock
    if (m_func_thread_active.load(std::memory_order_acquire)) {
        return VCI_UDS_RESULT_OK;
    }

    std::lock_guard<std::mutex> lock(m_func_thread_mutex);
    // Double-check after acquiring the lock
    if (m_func_thread_active.load(std::memory_order_acquire)) {
        return VCI_UDS_RESULT_OK;
    }

    if (m_func_processing_thread.joinable()) {
        m_func_processing_thread.join(); // Clean up previous instance
    }
    m_func_thread_active.store(true, std::memory_order_release);
    m_func_processing_thread = std::thread(&Service::functionalProcessingThreadFunc, this);
    return VCI_UDS_RESULT_OK;
}

int Service::stopFunctionalProcessingThread() {
    if (!m_func_thread_active.exchange(false, std::memory_order_acq_rel)) {
        return VCI_UDS_RESULT_OK; // Already stopped
    }

    m_functional_request_queue.enqueue({}); // Unblock the waiting thread

    std::lock_guard<std::mutex> lock(m_func_thread_mutex);
    if (m_func_processing_thread.joinable()) {
        m_func_processing_thread.join();
    }
    return VCI_UDS_RESULT_OK;
}

// --- Thread Implementations ---

void Service::physicalProcessingThreadFunc() {
    LOG_INFO("[VCI-UDS-SVC] Physical processing thread started (id: {}).", Common::Utils::getThreadIdString());

    while (m_service_is_running.load(std::memory_order_acquire) && m_phys_thread_active.load(std::memory_order_acquire)) {
        Request req;
        bool dequeued = m_request_queue.wait_dequeue_timed(req, std::chrono::minutes(ASYNC_THREAD_IDLE_TIMEOUT_MIN));

        if (!m_phys_thread_active.load(std::memory_order_acquire) || !m_service_is_running.load(std::memory_order_acquire)) {
            break; // Exit if stopped externally
        }

        if (!dequeued) {
            LOG_INFO("[VCI-UDS-SVC] Physical processing thread idle timeout after {} minutes. Exiting.", ASYNC_THREAD_IDLE_TIMEOUT_MIN);
            break; // Exit on idle timeout
        }

        if (req.payload.empty()) {
            continue; // This is the signal to unblock, but we might not be stopping yet, so re-check loop condition
        }

        Response res = executeTransaction(req.payload);
        if (res.result_code != VCI_UDS_RESULT_ABORTED) {
            m_response_queue.enqueue(std::move(res));
        }
    }

    m_phys_thread_active.store(false, std::memory_order_release);
    LOG_INFO("[VCI-UDS-SVC] Physical processing thread stopped (id: {}).", Common::Utils::getThreadIdString());
}

void Service::functionalProcessingThreadFunc() {
    LOG_INFO("[VCI-UDS-SVC] Functional processing thread started (id: {}).", Common::Utils::getThreadIdString());

    while (m_service_is_running.load(std::memory_order_acquire) && m_func_thread_active.load(std::memory_order_acquire)) {
        Request req;
        bool dequeued = m_functional_request_queue.wait_dequeue_timed(req, std::chrono::minutes(ASYNC_THREAD_IDLE_TIMEOUT_MIN));

        if (!m_func_thread_active.load(std::memory_order_acquire) || !m_service_is_running.load(std::memory_order_acquire)) {
            break; // Exit if stopped externally
        }

        if (!dequeued) {
            LOG_INFO("[VCI-UDS-SVC] Functional processing thread idle timeout after {} minutes. Exiting.", ASYNC_THREAD_IDLE_TIMEOUT_MIN);
            break; // Exit on idle timeout
        }

        if (req.payload.empty()) {
            continue;
        }

        std::vector<UdsFunctionalResponse> responses;
        auto context = getContext();
        auto sender = [this](const FkVciCanDataType& frame) { return m_communicator->sendFrame(frame); };
        auto provider = [this](uint64_t timeout) { return m_communicator->receiveFrame(timeout); };
        {
            std::lock_guard<std::mutex> ptr_lock(m_functional_pointer_mutex);
            m_functional_transaction = std::make_unique<tp::FunctionalTransaction>(context, sender, provider, req.payload);
        }

        {
            std::lock_guard<std::mutex> tx_lock(m_transaction_mutex);
            m_communicator->clearReceiver();
            responses = m_functional_transaction->execute();
        }

        {
            std::lock_guard<std::mutex> ptr_lock(m_functional_pointer_mutex);
            m_functional_transaction.reset();
        }

        for (auto& res : responses) {
            m_functional_response_queue.enqueue(std::move(res));
        }
    }

    m_func_thread_active.store(false, std::memory_order_release);
    LOG_INFO("[VCI-UDS-SVC] Functional processing thread stopped (id: {}).", Common::Utils::getThreadIdString());
}

Service::Response Service::executeTransaction(const std::vector<uint8_t>& payload) {
    updateLastTxTime(); // for keep-alive

    tp::TransactionResult internal_result;
    auto context = getContext();
    auto sender = [this](const FkVciCanDataType& frame) { updateLastTxTime(); return m_communicator->sendFrame(frame); };
    auto provider = [this](uint64_t timeout) { return m_communicator->receiveFrame(timeout); };

    // Create transaction object outside the main lock
    {
        std::lock_guard<std::mutex> ptr_lock(m_transaction_pointer_mutex);
        m_transaction = std::make_unique<tp::Transaction>(context, sender, provider, payload);
    }

    // Lock only for the duration of the actual CAN bus communication
    {
        std::lock_guard<std::mutex> tx_lock(m_transaction_mutex);
        m_communicator->clearReceiver();
        internal_result = m_transaction->execute();
    }

    // Reset transaction object outside the main lock
    {
        std::lock_guard<std::mutex> ptr_lock(m_transaction_pointer_mutex);
        m_transaction.reset();
    }

    return {internal_result.result_code, std::move(internal_result.response_payload)};
}

int Service::readResponse(std::vector<uint8_t>& response_payload, uint32_t timeout_ms) {
    Response res;
    if (m_response_queue.wait_dequeue_timed(res, std::chrono::milliseconds(timeout_ms))) {
        response_payload = std::move(res.payload);
        return res.result_code;
    }
    return VCI_UDS_RESULT_NO_RESPONSE_IN_QUEUE;
}

void Service::updateLastTxTime() {
    m_last_tx_time_ms.store(Common::Utils::getCurrentMillisecondsFast(), std::memory_order_release);
}

int Service::readFunctionalResponses(std::vector<UdsFunctionalResponse>& responses, uint32_t& max_count, uint32_t timeout_ms) {
    responses.clear();
    if (max_count > 0) responses.reserve(max_count);
    size_t dequeued_count = 0;
    if (timeout_ms > 0) {
        dequeued_count = m_functional_response_queue.wait_dequeue_bulk_timed(std::back_inserter(responses), max_count, std::chrono::milliseconds(timeout_ms));
    } else {
        dequeued_count = m_functional_response_queue.try_dequeue_bulk(std::back_inserter(responses), max_count);
    }
    max_count = static_cast<uint32_t>(dequeued_count);
    return  max_count == 0 ? VCI_UDS_RESULT_NO_RESPONSE_IN_QUEUE : VCI_UDS_RESULT_OK;
}

void Service::clearAsyncQueues() {
    m_request_queue = moodycamel::BlockingConcurrentQueue<Request>();
    m_response_queue = moodycamel::BlockingConcurrentQueue<Response>();
    m_functional_request_queue = moodycamel::BlockingConcurrentQueue<Request>();
    m_functional_response_queue = moodycamel::BlockingConcurrentQueue<UdsFunctionalResponse>();
}

void Service::abort() {
    {
        std::lock_guard<std::mutex> lock(m_transaction_pointer_mutex);
        if (m_transaction) m_transaction->stopExecution();
    }
    {
        std::lock_guard<std::mutex> lock(m_functional_pointer_mutex);
        if (m_functional_transaction) m_functional_transaction->stopExecution();
    }
}

int Service::startKeepAlive() {
    if (m_keep_alive_active.load(std::memory_order_acquire)) return VCI_UDS_RESULT_OK;
    m_keep_alive_active.store(true, std::memory_order_release);
    updateLastTxTime();
    m_keep_alive_thread = std::thread(&Service::keepAliveThreadFunc, this);
    return VCI_UDS_RESULT_OK;
}

int Service::stopKeepAlive() {
    if (!m_keep_alive_active.exchange(false, std::memory_order_acq_rel)) return VCI_UDS_RESULT_OK;
    m_keep_alive_cv.notify_one();
    if (m_keep_alive_thread.joinable()) {
        m_keep_alive_thread.join();
    }
    return VCI_UDS_RESULT_OK;
}

void Service::keepAliveThreadFunc() {
    LOG_INFO("[VCI-UDS-SVC] Keep-alive thread started (id: {}).", Common::Utils::getThreadIdString());
    while (m_keep_alive_active.load(std::memory_order_acquire)) {
        auto context = getContext();
        if (context.tester_present_interval_ms == 0) break;

        uint64_t now = Common::Utils::getCurrentMillisecondsFast();
        uint64_t last_tx = m_last_tx_time_ms.load(std::memory_order_acquire);
        uint64_t elapsed = (now > last_tx) ? (now - last_tx) : 0;
        if (elapsed < context.tester_present_interval_ms) {
            std::unique_lock<std::mutex> lock(m_keep_alive_mutex);
            m_keep_alive_cv.wait_for(lock, std::chrono::milliseconds(context.tester_present_interval_ms - elapsed), [this] { return !m_keep_alive_active.load(std::memory_order_acquire); });
            continue;
        }

        {
            std::lock_guard<std::mutex> tx_lock(m_transaction_mutex);
            now = Common::Utils::getCurrentMillisecondsFast();
            last_tx = m_last_tx_time_ms.load(std::memory_order_acquire);
            elapsed = (now > last_tx) ? (now - last_tx) : 0;
            if (elapsed < context.tester_present_interval_ms) continue;
            uint8_t payload[] = { 0x3E, context.tester_present_sub_func };
            UdsTp_Frame tp_frame;
            tp_frame.pci_type = UDS_TP_PCI_TYPE_SINGLE_FRAME;
            tp_frame.frame.sf.payload = payload;
            tp_frame.frame.sf.payload_size = sizeof(payload);
            uint32_t target_id = (context.tester_present_id != 0) ? context.tester_present_id : context.request_id;
            FkVciCanDataType can_frame = tp::Utils::build(tp_frame, target_id, context.can_type, context.padding_target_size, context.padding_fill_byte);
            if (m_communicator->sendFrame(can_frame)) {
                updateLastTxTime();
            } else {
                LOG_WARN("[VCI-UDS-SVC] Keep-alive: Failed to send TesterPresent.");
            }
        }
    }
    LOG_INFO("[VCI-UDS-SVC] Keep-alive thread stopped (id: {}).", Common::Utils::getThreadIdString());
}

} // namespace uds
} // namespace vci
