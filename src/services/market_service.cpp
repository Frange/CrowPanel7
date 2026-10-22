#include "market_service.h"

#include <Arduino.h>
#include <WiFi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "storage_service.h"

namespace {

constexpr size_t MAX_TICKERS = 40;
constexpr size_t SYMBOL_SIZE = 17;

enum JobType : uint8_t {
    JOB_REFRESH = 1,
    JOB_VALIDATE,
    JOB_HISTORY
};

struct MarketJob {
    uint8_t type;
    char symbol[SYMBOL_SIZE];
    uint8_t period;
};

struct CachedStock {
    bool occupied;
    StockData data;
};

QueueHandle_t job_queue = nullptr;
SemaphoreHandle_t market_mutex = nullptr;
TaskHandle_t market_task = nullptr;

CachedStock cache[MAX_TICKERS] = {};

bool initialized = false;
bool refresh_pending = false;
bool validation_pending = false;
bool validation_done = false;
bool history_pending = false;
bool history_done = false;
bool worker_busy = false;

String validation_symbol;
StockData validation_data = {};
bool validation_success = false;

HistoryData history_result = {};

void lock_market()
{
    if (market_mutex != nullptr) {
        xSemaphoreTake(market_mutex, portMAX_DELAY);
    }
}

void unlock_market()
{
    if (market_mutex != nullptr) {
        xSemaphoreGive(market_mutex);
    }
}

void cache_store(const StockData &data)
{
    if (!data.valid) {
        return;
    }

    lock_market();

    int free_slot = -1;

    for (size_t i = 0; i < MAX_TICKERS; ++i) {
        if (cache[i].occupied &&
            cache[i].data.symbol == data.symbol) {
            cache[i].data = data;
            unlock_market();
            return;
        }

        if (!cache[i].occupied && free_slot < 0) {
            free_slot = static_cast<int>(i);
        }
    }

    if (free_slot >= 0) {
        cache[free_slot].occupied = true;
        cache[free_slot].data = data;
    }

    unlock_market();
}

bool fetch_one(const String &symbol)
{
    StockData data = {};

    if (!api_fetch_stock_data(symbol.c_str(), &data) ||
        !data.valid) {
        Serial.printf(
            "[MARKET] Fallo al actualizar %s\n",
            symbol.c_str()
        );
        return false;
    }

    cache_store(data);
    return true;
}

void process_refresh()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[MARKET] Sin WiFi");
        return;
    }

    size_t count = storage_ticker_count();

    if (count > MAX_TICKERS) {
        count = MAX_TICKERS;
    }

    for (size_t i = 0; i < count; ++i) {
        String symbol = storage_get_ticker(i);
        symbol.trim();
        symbol.toUpperCase();

        if (!symbol.isEmpty()) {
            fetch_one(symbol);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }

    Serial.println("[MARKET] Ronda terminada");
}

void process_validation(const MarketJob &job)
{
    String symbol = String(job.symbol);
    StockData data = {};

    bool ok =
        api_fetch_stock_data(symbol.c_str(), &data) &&
        data.valid;

    if (ok) {
        cache_store(data);
    }

    lock_market();

    validation_symbol = symbol;
    validation_data = data;
    validation_success = ok;
    validation_done = true;
    validation_pending = false;

    unlock_market();
}

void process_history(const MarketJob &job)
{
    String symbol = String(job.symbol);
    HistoryData data = {};

    bool ok = api_fetch_stock_history(
        symbol.c_str(),
        static_cast<HistoryPeriod>(job.period),
        &data
    );

    if (!ok) {
        data.valid = false;
        data.count = 0;
        data.symbol = symbol;
        data.period =
            static_cast<HistoryPeriod>(job.period);
    }

    lock_market();

    history_result = data;
    history_done = true;
    history_pending = false;

    unlock_market();
}

void market_worker(void *parameter)
{
    (void)parameter;

    MarketJob job = {};

    for (;;) {
        if (xQueueReceive(
                job_queue,
                &job,
                portMAX_DELAY) != pdTRUE) {
            continue;
        }

        lock_market();
        worker_busy = true;
        unlock_market();

        switch (job.type) {
            case JOB_REFRESH:
                process_refresh();

                lock_market();
                refresh_pending = false;
                unlock_market();
                break;

            case JOB_VALIDATE:
                process_validation(job);
                break;

            case JOB_HISTORY:
                process_history(job);
                break;

            default:
                break;
        }

        lock_market();
        worker_busy = false;
        unlock_market();
    }
}

bool enqueue_job(const MarketJob &job)
{
    return xQueueSend(job_queue, &job, 0) == pdTRUE;
}

} // namespace

void market_service_init()
{
    if (initialized) {
        return;
    }

    market_mutex = xSemaphoreCreateMutex();

    if (market_mutex == nullptr) {
        Serial.println("[MARKET] Error creando mutex");
        return;
    }

    job_queue = xQueueCreate(4, sizeof(MarketJob));

    if (job_queue == nullptr) {
        vSemaphoreDelete(market_mutex);
        market_mutex = nullptr;
        return;
    }

    BaseType_t result = xTaskCreate(
        market_worker,
        "market_worker",
        12288,
        nullptr,
        1,
        &market_task
    );

    if (result != pdPASS) {
        vQueueDelete(job_queue);
        vSemaphoreDelete(market_mutex);
        job_queue = nullptr;
        market_mutex = nullptr;
        return;
    }

    initialized = true;
    Serial.println("[MARKET] Servicio inicializado");
}

bool market_service_request_refresh()
{
    if (!initialized) {
        return false;
    }

    lock_market();

    if (refresh_pending) {
        unlock_market();
        return false;
    }

    refresh_pending = true;
    unlock_market();

    MarketJob job = {};
    job.type = JOB_REFRESH;

    if (!enqueue_job(job)) {
        lock_market();
        refresh_pending = false;
        unlock_market();
        return false;
    }

    return true;
}

bool market_service_is_busy()
{
    if (!initialized) {
        return false;
    }

    lock_market();

    bool busy =
        worker_busy ||
        refresh_pending ||
        validation_pending ||
        history_pending;

    unlock_market();
    return busy;
}

bool market_service_get_data(
    const String &symbol,
    StockData &out)
{
    if (!initialized) {
        return false;
    }

    lock_market();

    for (size_t i = 0; i < MAX_TICKERS; ++i) {
        if (cache[i].occupied &&
            cache[i].data.symbol == symbol) {
            out = cache[i].data;
            unlock_market();
            return true;
        }
    }

    unlock_market();
    return false;
}

bool market_service_validate_async(
    const String &symbol)
{
    if (!initialized || symbol.isEmpty()) {
        return false;
    }

    lock_market();

    if (validation_pending || validation_done) {
        unlock_market();
        return false;
    }

    validation_pending = true;
    unlock_market();

    MarketJob job = {};
    job.type = JOB_VALIDATE;

    symbol.substring(0, SYMBOL_SIZE - 1)
          .toCharArray(job.symbol, sizeof(job.symbol));

    if (!enqueue_job(job)) {
        lock_market();
        validation_pending = false;
        unlock_market();
        return false;
    }

    return true;
}

bool market_service_take_validation_result(
    String &symbol,
    StockData &data,
    bool &success)
{
    if (!initialized) {
        return false;
    }

    lock_market();

    if (!validation_done) {
        unlock_market();
        return false;
    }

    symbol = validation_symbol;
    data = validation_data;
    success = validation_success;
    validation_done = false;

    unlock_market();
    return true;
}

bool market_service_request_history(
    const String &symbol,
    HistoryPeriod period)
{
    if (!initialized ||
        symbol.isEmpty() ||
        period >= HISTORY_PERIOD_COUNT) {
        return false;
    }

    lock_market();

    if (history_pending || history_done) {
        unlock_market();
        return false;
    }

    history_pending = true;
    unlock_market();

    MarketJob job = {};
    job.type = JOB_HISTORY;
    job.period = static_cast<uint8_t>(period);

    symbol.substring(0, SYMBOL_SIZE - 1)
          .toCharArray(job.symbol, sizeof(job.symbol));

    if (!enqueue_job(job)) {
        lock_market();
        history_pending = false;
        unlock_market();
        return false;
    }

    return true;
}

bool market_service_take_history_result(
    HistoryData &out)
{
    if (!initialized) {
        return false;
    }

    lock_market();

    if (!history_done) {
        unlock_market();
        return false;
    }

    out = history_result;
    history_done = false;

    unlock_market();
    return true;
}