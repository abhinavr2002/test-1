#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <set>
#include <queue>
#include <stack>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <chrono>
#include <ctime>
#include <cmath>
#include <numeric>
#include <functional>
#include <optional>
#include <variant>

// ============================================================================
// SECTION 1: CONSTANTS & CONFIGURATION
// ============================================================================

namespace SystemConfig {
    constexpr int MAX_BOOKS_PER_USER = 5;
    constexpr int LOAN_PERIOD_DAYS = 14;
    constexpr double LATE_FEE_PER_DAY = 0.50;
    constexpr int MAX_SYSTEM_ITEMS = 10000;
    constexpr const char* SYSTEM_VERSION = "4.2.0-dummy";
    constexpr const char* DEFAULT_BRANCH = "Central Main Branch";
}

// ============================================================================
// SECTION 2: ENUMERATIONS
// ============================================================================

enum class ItemCategory {
    FICTION,
    NON_FICTION,
    SCIENCE,
    HISTORY,
    TECHNOLOGY,
    ART,
    BIOGRAPHY,
    REFERENCE,
    PERIODICAL,
    UNKNOWN
};

enum class ItemStatus {
    AVAILABLE,
    CHECKED_OUT,
    RESERVED,
    MAINTENANCE,
    LOST,
    ARCHIVED
};

enum class UserRole {
    GUEST,
    MEMBER,
    RESEARCHER,
    LIBRARIAN,
    ADMINISTRATOR
};

enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

// ============================================================================
// SECTION 3: UTILITY FUNCTIONS & HELPERS
// ============================================================================

namespace Utils {

    std::string categoryToString(ItemCategory cat) {
        switch (cat) {
            case ItemCategory::FICTION:     return "Fiction";
            case ItemCategory::NON_FICTION: return "Non-Fiction";
            case ItemCategory::SCIENCE:     return "Science";
            case ItemCategory::HISTORY:     return "History";
            case ItemCategory::TECHNOLOGY:  return "Technology";
            case ItemCategory::ART:         return "Art";
            case ItemCategory::BIOGRAPHY:   return "Biography";
            case ItemCategory::REFERENCE:   return "Reference";
            case ItemCategory::PERIODICAL:  return "Periodical";
            default:                        return "Unknown";
        }
    }

    std::string statusToString(ItemStatus status) {
        switch (status) {
            case ItemStatus::AVAILABLE:   return "Available";
            case ItemStatus::CHECKED_OUT: return "Checked Out";
            case ItemStatus::RESERVED:    return "Reserved";
            case ItemStatus::MAINTENANCE: return "Maintenance";
            case ItemStatus::LOST:        return "Lost";
            case ItemStatus::ARCHIVED:    return "Archived";
            default:                      return "Unknown";
        }
    }

    std::string roleToString(UserRole role) {
        switch (role) {
            case UserRole::GUEST:         return "Guest";
            case UserRole::MEMBER:        return "Member";
            case UserRole::RESEARCHER:    return "Researcher";
            case UserRole::LIBRARIAN:     return "Librarian";
            case UserRole::ADMINISTRATOR: return "Administrator";
            default:                      return "None";
        }
    }

    std::string toLowerCase(const std::string& input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::tolower(c); });
        return result;
    }

    std::string toUpperCase(const std::string& input) {
        std::string result = input;
        std::transform(result.begin(), result.end(), result.begin(),
                       [](unsigned char c) { return std::toupper(c); });
        return result;
    }

    std::string trimWhitespace(const std::string& str) {
        size_t first = str.find_first_not_of(" \t\n\r");
        if (std::string::npos == first) {
            return "";
        }
        size_t last = str.find_last_not_of(" \t\n\r");
        return str.substr(first, (last - first + 1));
    }

    std::vector<std::string> splitString(const std::string& str, char delimiter) {
        std::vector<std::string> tokens;
        std::string token;
        std::istringstream tokenStream(str);
        while (std::getline(tokenStream, token, delimiter)) {
            tokens.push_back(trimWhitespace(token));
        }
        return tokens;
    }

    std::string generateTimestamp() {
        auto now = std::chrono::system_clock::now();
        std::time_t now_c = std::chrono::system_clock::to_time_t(now);
        std::tm* parts = std::localtime(&now_c);
        std::ostringstream oss;
        oss << std::put_time(parts, "%Y-%m-%d %H:%M:%S");
        return oss.str();
    }

    bool isValidISBN(const std::string& isbn) {
        std::string cleaned;
        for (char c : isbn) {
            if (std::isdigit(c) || c == 'X' || c == 'x') {
                cleaned += c;
            }
        }
        return (cleaned.length() == 10 || cleaned.length() == 13);
    }

    double roundCurrency(double value) {
        return std::round(value * 100.0) / 100.0;
    }

    std::string formatCurrency(double amount) {
        std::ostringstream oss;
        oss << "$" << std::fixed << std::setprecision(2) << roundCurrency(amount);
        return oss.str();
    }
}

// ============================================================================
// SECTION 4: CUSTOM EXCEPTIONS
// ============================================================================

class SystemException : public std::runtime_error {
protected:
    int errorCode;
public:
    SystemException(const std::string& msg, int code = 1000)
        : std::runtime_error(msg), errorCode(code) {}
    int getErrorCode() const noexcept { return errorCode; }
};

class ItemNotFoundException : public SystemException {
public:
    explicit ItemNotFoundException(const std::string& id)
        : SystemException("Item not found with ID: " + id, 1001) {}
};

class UserNotFoundException : public SystemException {
public:
    explicit UserNotFoundException(const std::string& id)
        : SystemException("User not found with ID: " + id, 1002) {}
};

class ItemUnavailableException : public SystemException {
public:
    explicit ItemUnavailableException(const std::string& title)
        : SystemException("Item is currently unavailable: " + title, 1003) {}
};

class QuotaExceededException : public SystemException {
public:
    explicit QuotaExceededException(const std::string& userId)
        : SystemException("Borrowing quota exceeded for user: " + userId, 1004) {}
};

class PermissionDeniedException : public SystemException {
public:
    explicit PermissionDeniedException(const std::string& action)
        : SystemException("Permission denied for action: " + action, 1005) {}
};

// ============================================================================
// SECTION 5: LOGGING SUBSYSTEM
// ============================================================================

struct LogEntry {
    std::string timestamp;
    LogLevel level;
    std::string module;
    std::string message;
};

class SystemLogger {
private:
    std::vector<LogEntry> logs;
    LogLevel minLevel;
    bool consoleOutput;

    std::string levelToString(LogLevel lvl) const {
        switch (lvl) {
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARN";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
            default:                 return "UNKNOWN";
        }
    }

public:
    SystemLogger(LogLevel minimum = LogLevel::INFO, bool echo = false)
        : minLevel(minimum), consoleOutput(echo) {}

    void setMinLevel(LogLevel lvl) { minLevel = lvl; }
    void setConsoleOutput(bool echo) { consoleOutput = echo; }

    void log(LogLevel lvl, const std::string& module, const std::string& msg) {
        if (lvl < minLevel) return;
        LogEntry entry{Utils::generateTimestamp(), lvl, module, msg};
        logs.push_back(entry);
        if (consoleOutput) {
            std::cout << "[" << entry.timestamp << "] ["
                      << levelToString(lvl) << "] ["
                      << module << "] " << msg << "\n";
        }
    }

    void debug(const std::string& mod, const std::string& msg) { log(LogLevel::DEBUG, mod, msg); }
    void info(const std::string& mod, const std::string& msg) { log(LogLevel::INFO, mod, msg); }
    void warn(const std::string& mod, const std::string& msg) { log(LogLevel::WARNING, mod, msg); }
    void error(const std::string& mod, const std::string& msg) { log(LogLevel::ERROR, mod, msg); }
    void critical(const std::string& mod, const std::string& msg) { log(LogLevel::CRITICAL, mod, msg); }

    size_t getLogCount() const { return logs.size(); }

    std::vector<LogEntry> getLogsByLevel(LogLevel lvl) const {
        std::vector<LogEntry> filtered;
        for (const auto& entry : logs) {
            if (entry.level == lvl) {
                filtered.push_back(entry);
            }
        }
        return filtered;
    }

    void clearLogs() { logs.clear(); }
};

// ============================================================================
// SECTION 6: BASE & DERIVED LIBRARY ITEM MODELS
// ============================================================================

class LibraryItem {
protected:
    std::string itemId;
    std::string title;
    int publicationYear;
    ItemCategory category;
    ItemStatus status;
    std::string locationCode;
    int borrowCount;
    double replacementCost;

public:
    LibraryItem(std::string id, std::string t, int year, ItemCategory cat,
                std::string loc = "GEN-01", double cost = 25.0)
        : itemId(std::move(id)), title(std::move(t)), publicationYear(year),
          category(cat), status(ItemStatus::AVAILABLE),
          locationCode(std::move(loc)), borrowCount(0), replacementCost(cost) {}

    virtual ~LibraryItem() = default;

    std::string getId() const { return itemId; }
    std::string getTitle() const { return title; }
    int getPublicationYear() const { return publicationYear; }
    ItemCategory getCategory() const { return category; }
    ItemStatus getStatus() const { return status; }
    std::string getLocationCode() const { return locationCode; }
    int getBorrowCount() const { return borrowCount; }
    double getReplacementCost() const { return replacementCost; }

    void setStatus(ItemStatus newStatus) { status = newStatus; }
    void setLocationCode(const std::string& loc) { locationCode = loc; }
    void incrementBorrowCount() { ++borrowCount; }

    virtual std::string getItemType() const = 0;
    virtual int getMaxLoanDays() const { return SystemConfig::LOAN_PERIOD_DAYS; }

    virtual std::string getSummary() const {
        std::ostringstream oss;
        oss << "[" << getItemType() << "] " << itemId << " - \"" << title
            << "\" (" << publicationYear << ") | Status: "
            << Utils::statusToString(status);
        return oss.str();
    }
};

class Book : public LibraryItem {
private:
    std::string author;
    std::string isbn;
    std::string publisher;
    int pageCount;
    bool hardcover;

public:
    Book(std::string id, std::string t, std::string auth, std::string isbnCode,
         int year, ItemCategory cat, int pages = 300, bool isHardcover = true,
         std::string pub = "Standard Press", double cost = 30.0)
        : LibraryItem(std::move(id), std::move(t), year, cat, "BK-SHELF", cost),
          author(std::move(auth)), isbn(std::move(isbnCode)),
          publisher(std::move(pub)), pageCount(pages), hardcover(isHardcover) {}

    std::string getAuthor() const { return author; }
    std::string getISBN() const { return isbn; }
    std::string getPublisher() const { return publisher; }
    int getPageCount() const { return pageCount; }
    bool isHardcover() const { return hardcover; }

    std::string getItemType() const override { return "Book"; }

    std::string getSummary() const override {
        std::ostringstream oss;
        oss << LibraryItem::getSummary() << " | Author: " << author
            << " | ISBN: " << isbn << " | Pages: " << pageCount;
        return oss.str();
    }
};

class DigitalMedia : public LibraryItem {
private:
    std::string format;
    double fileSizeMB;
    int durationMinutes;
    bool drmProtected;

public:
    DigitalMedia(std::string id, std::string t, int year, ItemCategory cat,
                 std::string fmt, double sizeMB, int duration, bool drm = true)
        : LibraryItem(std::move(id), std::move(t), year, cat, "DIG-SERVER", 15.0),
          format(std::move(fmt)), fileSizeMB(sizeMB),
          durationMinutes(duration), drmProtected(drm) {}

    std::string getFormat() const { return format; }
    double getFileSizeMB() const { return fileSizeMB; }
    int getDurationMinutes() const { return durationMinutes; }
    bool hasDRM() const { return drmProtected; }

    std::string getItemType() const override { return "DigitalMedia"; }
    int getMaxLoanDays() const override { return 7; }

    std::string getSummary() const override {
        std::ostringstream oss;
        oss << LibraryItem::getSummary() << " | Format: " << format
            << " | Size: " << fileSizeMB << "MB | Duration: " << durationMinutes << "m";
        return oss.str();
    }
};

class ResearchJournal : public LibraryItem {
private:
    std::string issn;
    int volume;
    int issueNumber;
    bool peerReviewed;

public:
    ResearchJournal(std::string id, std::string t, int year, std::string issnCode,
                    int vol, int issue, bool peerRev = true)
        : LibraryItem(std::move(id), std::move(t), year, ItemCategory::PERIODICAL, "JRN-ARCHIVE", 45.0),
          issn(std::move(issnCode)), volume(vol), issueNumber(issue), peerReviewed(peerRev) {}

    std::string getISSN() const { return issn; }
    int getVolume() const { return volume; }
    int getIssueNumber() const { return issueNumber; }
    bool isPeerReviewed() const { return peerReviewed; }

    std::string getItemType() const override { return "ResearchJournal"; }
    int getMaxLoanDays() const override { return 3; }

    std::string getSummary() const override {
        std::ostringstream oss;
        oss << LibraryItem::getSummary() << " | ISSN: " << issn
            << " | Vol." << volume << " No." << issueNumber;
        return oss.str();
    }
};

// ============================================================================
// SECTION 7: USER & ACCOUNT MODELS
// ============================================================================

struct LoanRecord {
    std::string transactionId;
    std::string userId;
    std::string itemId;
    int dayCheckedOut;
    int dueDay;
    int dayReturned;
    bool isReturned;
    double fineAccrued;
};

class UserAccount {
private:
    std::string userId;
    std::string fullName;
    std::string email;
    UserRole role;
    bool active;
    double balanceDue;
    std::vector<std::string> activeItemIds;
    std::vector<LoanRecord> loanHistory;

public:
    UserAccount(std::string id, std::string name, std::string mail, UserRole r = UserRole::MEMBER)
        : userId(std::move(id)), fullName(std::move(name)), email(std::move(mail)),
          role(r), active(true), balanceDue(0.0) {}

    std::string getUserId() const { return userId; }
    std::string getFullName() const { return fullName; }
    std::string getEmail() const { return email; }
    UserRole getRole() const { return role; }
    bool isActive() const { return active; }
    double getBalanceDue() const { return balanceDue; }
    const std::vector<std::string>& getActiveItemIds() const { return activeItemIds; }
    const std::vector<LoanRecord>& getLoanHistory() const { return loanHistory; }

    void setActive(bool status) { active = status; }
    void setRole(UserRole newRole) { role = newRole; }

    int getMaxBorrowLimit() const {
        switch (role) {
            case UserRole::GUEST:         return 1;
            case UserRole::MEMBER:        return SystemConfig::MAX_BOOKS_PER_USER;
            case UserRole::RESEARCHER:    return 15;
            case UserRole::LIBRARIAN:     return 20;
            case UserRole::ADMINISTRATOR: return 25;
            default:                      return 0;
        }
    }

    bool canBorrow() const {
        return active && (balanceDue < 10.0) &&
               (static_cast<int>(activeItemIds.size()) < getMaxBorrowLimit());
    }

    void addBorrowedItem(const std::string& itemId, const LoanRecord& record) {
        activeItemIds.push_back(itemId);
        loanHistory.push_back(record);
    }

    void removeBorrowedItem(const std::string& itemId, int returnDay, double fine) {
        auto it = std::find(activeItemIds.begin(), activeItemIds.end(), itemId);
        if (it != activeItemIds.end()) {
            activeItemIds.erase(it);
        }
        for (auto& rec : loanHistory) {
            if (rec.itemId == itemId && !rec.isReturned) {
                rec.isReturned = true;
                rec.dayReturned = returnDay;
                rec.fineAccrued = fine;
                break;
            }
        }
        balanceDue += fine;
    }

    void payFine(double amount) {
        if (amount > 0) {
            balanceDue = std::max(0.0, balanceDue - amount);
        }
    }
};

// ============================================================================
// SECTION 8: GENERIC REPOSITORY TEMPLATE
// ============================================================================

template <typename T>
class Repository {
private:
    std::unordered_map<std::string, std::shared_ptr<T>> storage;

public:
    void add(const std::string& id, std::shared_ptr<T> entity) {
        storage[id] = std::move(entity);
    }

    bool remove(const std::string& id) {
        return storage.erase(id) > 0;
    }

    bool exists(const std::string& id) const {
        return storage.find(id) != storage.end();
    }

    std::shared_ptr<T> get(const std::string& id) const {
        auto it = storage.find(id);
        if (it == storage.end()) {
            return nullptr;
        }
        return it->second;
    }

    size_t count() const {
        return storage.size();
    }

    std::vector<std::shared_ptr<T>> getAll() const {
        std::vector<std::shared_ptr<T>> items;
        items.reserve(storage.size());
        for (const auto& pair : storage) {
            items.push_back(pair.second);
        }
        return items;
    }

    std::vector<std::shared_ptr<T>> findIf(std::function<bool(const std::shared_ptr<T>&)> predicate) const {
        std::vector<std::shared_ptr<T>> results;
        for (const auto& pair : storage) {
            if (predicate(pair.second)) {
                results.push_back(pair.second);
            }
        }
        return results;
    }

    void clear() {
        storage.clear();
    }
};

// ============================================================================
// SECTION 9: SEARCH & ANALYTICS ENGINE
// ============================================================================

class SearchEngine {
public:
    static std::vector<std::shared_ptr<LibraryItem>> searchByTitle(
        const Repository<LibraryItem>& repo, const std::string& query) {
        std::string lowerQuery = Utils::toLowerCase(Utils::trimWhitespace(query));
        return repo.findIf([&lowerQuery](const std::shared_ptr<LibraryItem>& item) {
            return Utils::toLowerCase(item->getTitle()).find(lowerQuery) != std::string::npos;
        });
    }

    static std::vector<std::shared_ptr<LibraryItem>> filterByCategory(
        const Repository<LibraryItem>& repo, ItemCategory category) {
        return repo.findIf([category](const std::shared_ptr<LibraryItem>& item) {
            return item->getCategory() == category;
        });
    }

    static std::vector<std::shared_ptr<LibraryItem>> filterByStatus(
        const Repository<LibraryItem>& repo, ItemStatus status) {
        return repo.findIf([status](const std::shared_ptr<LibraryItem>& item) {
            return item->getStatus() == status;
        });
    }

    static std::vector<std::shared_ptr<LibraryItem>> getTopBorrowedItems(
        const Repository<LibraryItem>& repo, size_t limit = 5) {
        auto allItems = repo.getAll();
        std::sort(allItems.begin(), allItems.end(),
                  [](const std::shared_ptr<LibraryItem>& a, const std::shared_ptr<LibraryItem>& b) {
                      return a->getBorrowCount() > b->getBorrowCount();
                  });
        if (allItems.size() > limit) {
            allItems.resize(limit);
        }
        return allItems;
    }
};

struct SystemReport {
    int totalItems;
    int availableItems;
    int checkedOutItems;
    int totalUsers;
    int activeUsers;
    double totalOutstandingFines;
    double totalInventoryValue;
    std::map<ItemCategory, int> itemsPerCategory;
};

class AnalyticsEngine {
public:
    static SystemReport generateReport(const Repository<LibraryItem>& items,
                                       const Repository<UserAccount>& users) {
        SystemReport report{};
        auto allItems = items.getAll();
        auto allUsers = users.getAll();

        report.totalItems = static_cast<int>(allItems.size());
        report.totalUsers = static_cast<int>(allUsers.size());

        for (const auto& item : allItems) {
            if (item->getStatus() == ItemStatus::AVAILABLE) {
                report.availableItems++;
            } else if (item->getStatus() == ItemStatus::CHECKED_OUT) {
                report.checkedOutItems++;
            }
            report.totalInventoryValue += item->getReplacementCost();
            report.itemsPerCategory[item->getCategory()]++;
        }

        for (const auto& user : allUsers) {
            if (user->isActive()) {
                report.activeUsers++;
            }
            report.totalOutstandingFines += user->getBalanceDue();
        }

        return report;
    }

    static void printReport(const SystemReport& report) {
        std::cout << "\n================ SYSTEM ANALYTICS REPORT ================\n";
        std::cout << "Total Catalog Items    : " << report.totalItems << "\n";
        std::cout << "Available Items        : " << report.availableItems << "\n";
        std::cout << "Checked Out Items      : " << report.checkedOutItems << "\n";
        std::cout << "Registered Users       : " << report.totalUsers << "\n";
        std::cout << "Active Users           : " << report.activeUsers << "\n";
        std::cout << "Total Inventory Value  : " << Utils::formatCurrency(report.totalInventoryValue) << "\n";
        std::cout << "Outstanding User Fines : " << Utils::formatCurrency(report.totalOutstandingFines) << "\n";
        std::cout << "--- Breakdown by Category ---\n";
        for (const auto& pair : report.itemsPerCategory) {
            std::cout << "  * " << std::left << std::setw(15)
                      << Utils::categoryToString(pair.first)
                      << ": " << pair.second << "\n";
        }
        std::cout << "=========================================================\n\n";
    }
};

// ============================================================================
// SECTION 10: RESERVATION QUEUE MANAGER
// ============================================================================

class ReservationManager {
private:
    std::unordered_map<std::string, std::queue<std::string>> waitlists;

public:
    void addReservation(const std::string& itemId, const std::string& userId) {
        waitlists[itemId].push(userId);
    }

    bool hasReservations(const std::string& itemId) const {
        auto it = waitlists.find(itemId);
        return it != waitlists.end() && !it->second.empty();
    }

    std::optional<std::string> popNextUser(const std::string& itemId) {
        auto it = waitlists.find(itemId);
        if (it == waitlists.end() || it->second.empty()) {
            return std::nullopt;
        }
        std::string nextUser = it->second.front();
        it->second.pop();
        return nextUser;
    }

    size_t getQueueLength(const std::string& itemId) const {
        auto it = waitlists.find(itemId);
        if (it == waitlists.end()) return 0;
        return it->second.size();
    }
};

// ============================================================================
// SECTION 11: CORE LIBRARY CONTROLLER
// ============================================================================

class LibraryController {
private:
    std::string branchName;
    Repository<LibraryItem> itemRepo;
    Repository<UserAccount> userRepo;
    ReservationManager reservations;
    SystemLogger logger;
    int currentSimulationDay;
    int transactionCounter;

    std::string nextTransactionId() {
        ++transactionCounter;
        std::ostringstream oss;
        oss << "TXN-" << std::setw(6) << std::setfill('0') << transactionCounter;
        return oss.str();
    }

public:
    explicit LibraryController(std::string branch = SystemConfig::DEFAULT_BRANCH)
        : branchName(std::move(branch)),
          logger(LogLevel::INFO, false),
          currentSimulationDay(1),
          transactionCounter(0) {
        logger.info("Controller", "Initialized branch: " + branchName);
    }

    void enableConsoleLogs(bool enable) {
        logger.setConsoleOutput(enable);
    }

    void advanceDays(int days) {
        if (days > 0) {
            currentSimulationDay += days;
            logger.info("Clock", "Advanced simulation day to " + std::to_string(currentSimulationDay));
        }
    }

    int getCurrentDay() const { return currentSimulationDay; }

    void registerItem(std::shared_ptr<LibraryItem> item) {
        if (!item) return;
        itemRepo.add(item->getId(), item);
        logger.info("Catalog", "Registered item: " + item->getId() + " (" + item->getTitle() + ")");
    }

    void registerUser(std::shared_ptr<UserAccount> user) {
        if (!user) return;
        userRepo.add(user->getUserId(), user);
        logger.info("Users", "Registered user: " + user->getUserId() + " (" + user->getFullName() + ")");
    }

    std::string checkoutItem(const std::string& userId, const std::string& itemId) {
        auto user = userRepo.get(userId);
        if (!user) {
            logger.error("Checkout", "Unknown user: " + userId);
            throw UserNotFoundException(userId);
        }

        auto item = itemRepo.get(itemId);
        if (!item) {
            logger.error("Checkout", "Unknown item: " + itemId);
            throw ItemNotFoundException(itemId);
        }

        if (item->getStatus() != ItemStatus::AVAILABLE) {
            logger.warn("Checkout", "Item unavailable: " + itemId);
            throw ItemUnavailableException(item->getTitle());
        }

        if (!user->canBorrow()) {
            logger.warn("Checkout", "User quota/status block: " + userId);
            throw QuotaExceededException(userId);
        }

        std::string txnId = nextTransactionId();
        int due = currentSimulationDay + item->getMaxLoanDays();

        LoanRecord record{
            txnId,
            userId,
            itemId,
            currentSimulationDay,
            due,
            -1,
            false,
            0.0
        };

        item->setStatus(ItemStatus::CHECKED_OUT);
        item->incrementBorrowCount();
        user->addBorrowedItem(itemId, record);

        logger.info("Checkout", "Success " + txnId + ": User " + userId + " borrowed " + itemId);
        return txnId;
    }

    double returnItem(const std::string& userId, const std::string& itemId) {
        auto user = userRepo.get(userId);
        if (!user) throw UserNotFoundException(userId);

        auto item = itemRepo.get(itemId);
        if (!item) throw ItemNotFoundException(itemId);

        double fine = 0.0;
        for (const auto& rec : user->getLoanHistory()) {
            if (rec.itemId == itemId && !rec.isReturned) {
                if (currentSimulationDay > rec.dueDay) {
                    int overdueDays = currentSimulationDay - rec.dueDay;
                    fine = overdueDays * SystemConfig::LATE_FEE_PER_DAY;
                }
                break;
            }
        }

        user->removeBorrowedItem(itemId, currentSimulationDay, fine);

        if (reservations.hasReservations(itemId)) {
            item->setStatus(ItemStatus::RESERVED);
            auto nextUser = reservations.popNextUser(itemId);
            if (nextUser.has_value()) {
                logger.info("Return", "Item " + itemId + " held for reserved user " + nextUser.value());
            }
        } else {
            item->setStatus(ItemStatus::AVAILABLE);
        }

        logger.info("Return", "User " + userId + " returned " + itemId + " | Fine: " + Utils::formatCurrency(fine));
        return fine;
    }

    void placeReservation(const std::string& userId, const std::string& itemId) {
        if (!userRepo.exists(userId)) throw UserNotFoundException(userId);
        if (!itemRepo.exists(itemId)) throw ItemNotFoundException(itemId);
        reservations.addReservation(itemId, userId);
        logger.info("Reservation", "User " + userId + " reserved item " + itemId);
    }

    std::vector<std::shared_ptr<LibraryItem>> searchCatalog(const std::string& keyword) const {
        return SearchEngine::searchByTitle(itemRepo, keyword);
    }

    void printFullDashboard() const {
        SystemReport report = AnalyticsEngine::generateReport(itemRepo, userRepo);
        AnalyticsEngine::printReport(report);

        std::cout << "--- Top Borrowed Items ---\n";
        auto topItems = SearchEngine::getTopBorrowedItems(itemRepo, 3);
        for (size_t i = 0; i < topItems.size(); ++i) {
            std::cout << (i + 1) << ". " << topItems[i]->getSummary()
                      << " (Borrows: " << topItems[i]->getBorrowCount() << ")\n";
        }
        std::cout << "\n";
    }

    size_t getTotalLogs() const {
        return logger.getLogCount();
    }
};

// ============================================================================
// SECTION 12: DUMMY DATA GENERATOR
// ============================================================================

class DummyDataSeeder {
public:
    static void populateLibrary(LibraryController& controller) {
        // Seed Books
        controller.registerItem(std::make_shared<Book>(
            "BK-101", "The C++ Programming Language", "Bjarne Stroustrup",
            "978-0321563842", 2013, ItemCategory::TECHNOLOGY, 1376, true, "Addison-Wesley", 65.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-102", "Effective Modern C++", "Scott Meyers",
            "978-1491903995", 2014, ItemCategory::TECHNOLOGY, 334, false, "O'Reilly Media", 45.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-103", "Clean Code", "Robert C. Martin",
            "978-0132350884", 2008, ItemCategory::TECHNOLOGY, 464, false, "Prentice Hall", 40.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-104", "A Brief History of Time", "Stephen Hawking",
            "978-0553380163", 1998, ItemCategory::SCIENCE, 212, false, "Bantam", 18.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-105", "Dune", "Frank Herbert",
            "978-0441172719", 1965, ItemCategory::FICTION, 688, true, "Ace Books", 25.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-106", "Sapiens: A Brief History of Humankind", "Yuval Noah Harari",
            "978-0062316097", 2015, ItemCategory::HISTORY, 443, true, "Harper", 28.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-107", "The Art of Computer Programming Vol. 1", "Donald Knuth",
            "978-0201896831", 1997, ItemCategory::TECHNOLOGY, 672, true, "Addison-Wesley", 75.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-108", "Foundation", "Isaac Asimov",
            "978-0553293357", 1951, ItemCategory::FICTION, 255, false, "Spectra", 16.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-109", "The Story of Art", "E.H. Gombrich",
            "978-0714832470", 1995, ItemCategory::ART, 688, true, "Phaidon Press", 55.00));

        controller.registerItem(std::make_shared<Book>(
            "BK-110", "Steve Jobs", "Walter Isaacson",
            "978-1451648539", 2011, ItemCategory::BIOGRAPHY, 656, true, "Simon & Schuster", 32.00));

        // Seed Digital Media
        controller.registerItem(std::make_shared<DigitalMedia>(
            "DM-201", "CppCon Keynote Archive 2024", 2024, ItemCategory::TECHNOLOGY,
            "MP4", 1450.5, 95, false));

        controller.registerItem(std::make_shared<DigitalMedia>(
            "DM-202", "Cosmos: A Spacetime Odyssey", 2014, ItemCategory::SCIENCE,
            "MKV", 4200.0, 550, true));

        controller.registerItem(std::make_shared<DigitalMedia>(
            "DM-203", "Symphony No. 9 Masterclass", 2021, ItemCategory::ART,
            "FLAC", 620.0, 72, false));

        // Seed Research Journals
        controller.registerItem(std::make_shared<ResearchJournal>(
            "JR-301", "Journal of Systems Architecture", 2025, "1383-7621", 148, 4, true));

        controller.registerItem(std::make_shared<ResearchJournal>(
            "JR-302", "Quarterly Review of Astrophysics", 2025, "0004-637X", 92, 2, true));

        // Seed Users
        controller.registerUser(std::make_shared<UserAccount>(
            "USR-001", "Alice Johnson", "alice@example.com", UserRole::MEMBER));

        controller.registerUser(std::make_shared<UserAccount>(
            "USR-002", "Bob Smith", "bob@example.com", UserRole::RESEARCHER));

        controller.registerUser(std::make_shared<UserAccount>(
            "USR-003", "Charlie Davis", "charlie@example.com", UserRole::GUEST));

        controller.registerUser(std::make_shared<UserAccount>(
            "USR-004", "Diana Prince", "diana@example.com", UserRole::LIBRARIAN));

        controller.registerUser(std::make_shared<UserAccount>(
            "USR-005", "Evan Wright", "evan@example.com", UserRole::ADMINISTRATOR));
    }
};

// ============================================================================
// SECTION 13: AUTOMATED SELF-TEST & SIMULATION HARNESS
// ============================================================================

class SimulationRunner {
private:
    LibraryController& controller;
    int passedTests;
    int failedTests;

    void assertCondition(bool condition, const std::string& testName) {
        if (condition) {
            ++passedTests;
            std::cout << "[PASS] " << testName << "\n";
        } else {
            ++failedTests;
            std::cout << "[FAIL] " << testName << "\n";
        }
    }

public:
    explicit SimulationRunner(LibraryController& ctrl)
        : controller(ctrl), passedTests(0), failedTests(0) {}

    void runAllTests() {
        std::cout << "Starting Dummy System Verification Suite...\n\n";

        // Test 1: Search functionality
        auto cppResults = controller.searchCatalog("C++");
        assertCondition(cppResults.size() == 2, "Search catalog for 'C++' returns 2 books");

        // Test 2: Standard checkout
        std::string txn1 = controller.checkoutItem("USR-001", "BK-101");
        assertCondition(!txn1.empty(), "Valid checkout generates transaction ID");

        // Test 3: Checkout unavailable item throws exception
        bool caughtUnavailable = false;
        try {
            controller.checkoutItem("USR-002", "BK-101");
        } catch (const ItemUnavailableException&) {
            caughtUnavailable = true;
        }
        assertCondition(caughtUnavailable, "Checking out unavailable item throws ItemUnavailableException");

        // Test 4: Reservation queue
        controller.placeReservation("USR-002", "BK-101");
        controller.advanceDays(5);
        double fine1 = controller.returnItem("USR-001", "BK-101");
        assertCondition(fine1 == 0.0, "On-time return incurs $0.00 fine");

        // Test 5: Overdue return calculates fine
        controller.checkoutItem("USR-003", "BK-105");
        controller.advanceDays(20); // 6 days overdue (14 day limit)
        double fine2 = controller.returnItem("USR-003", "BK-105");
        assertCondition(std::abs(fine2 - 3.00) < 0.001, "6 days overdue incurs $3.00 fine");

        // Test 6: Guest quota enforcement
        controller.checkoutItem("USR-003", "BK-108");
        bool caughtQuota = false;
        try {
            controller.checkoutItem("USR-003", "BK-104");
        } catch (const QuotaExceededException&) {
            caughtQuota = true;
        }
        assertCondition(caughtQuota, "Guest user exceeding 1-item quota throws QuotaExceededException");

        // Test 7: Multiple checkouts for Researcher
        controller.checkoutItem("USR-002", "BK-102");
        controller.checkoutItem("USR-002", "BK-103");
        controller.checkoutItem("USR-002", "JR-301");
        assertCondition(true, "Researcher successfully checks out multiple item types");

        // Test 8: Utility checks
        assertCondition(Utils::isValidISBN("978-0321563842"), "Valid 13-digit ISBN check passes");
        assertCondition(!Utils::isValidISBN("12345"), "Invalid ISBN check fails properly");

        std::cout << "\nTest Summary: " << passedTests << " Passed, "
                  << failedTests << " Failed.\n";
    }
};

// ============================================================================
// SECTION 14: MATRIX & NUMERIC WORKLOAD MODULE (FOR CPU/COMPILER TESTING)
// ============================================================================

namespace NumericBenchmark {

    class Matrix2D {
    private:
        size_t rows;
        size_t cols;
        std::vector<double> data;

    public:
        Matrix2D(size_t r, size_t c, double initialVal = 0.0)
            : rows(r), cols(c), data(r * c, initialVal) {}

        double& at(size_t r, size_t c) {
            if (r >= rows || c >= cols) {
                throw std::out_of_range("Matrix index out of bounds");
            }
            return data[r * cols + c];
        }

        double at(size_t r, size_t c) const {
            if (r >= rows || c >= cols) {
                throw std::out_of_range("Matrix index out of bounds");
            }
            return data[r * cols + c];
        }

        size_t getRows() const { return rows; }
        size_t getCols() const { return cols; }

        Matrix2D multiply(const Matrix2D& other) const {
            if (cols != other.rows) {
                throw std::invalid_argument("Incompatible matrix dimensions for multiplication");
            }
            Matrix2D result(rows, other.cols, 0.0);
            for (size_t i = 0; i < rows; ++i) {
                for (size_t k = 0; k < cols; ++k) {
                    double temp = at(i, k);
                    for (size_t j = 0; j < other.cols; ++j) {
                        result.at(i, j) += temp * other.at(k, j);
                    }
                }
            }
            return result;
        }

        double trace() const {
            size_t limit = std::min(rows, cols);
            double sum = 0.0;
            for (size_t i = 0; i < limit; ++i) {
                sum += at(i, i);
            }
            return sum;
        }
    };

    double runSyntheticWorkload(size_t dimension) {
        Matrix2D matA(dimension, dimension, 1.5);
        Matrix2D matB(dimension, dimension, 2.0);
        for (size_t i = 0; i < dimension; ++i) {
            matA.at(i, i) = static_cast<double>(i + 1);
            matB.at(i, dimension - 1 - i) = 0.5;
        }
        Matrix2D product = matA.multiply(matB);
        return product.trace();
    }
}

// ============================================================================
// SECTION 15: MAIN ENTRY POINT
// ============================================================================

int main() {
    std::cout << "=========================================================\n";
    std::cout << "  C++ DUMMY APPLICATION - VERSION " << SystemConfig::SYSTEM_VERSION << "\n";
    std::cout << "=========================================================\n\n";

    try {
        LibraryController library("Downtown Tech & Science Library");
        library.enableConsoleLogs(false);

        DummyDataSeeder::populateLibrary(library);

        SimulationRunner tester(library);
        tester.runAllTests();

        library.printFullDashboard();

        double traceResult = NumericBenchmark::runSyntheticWorkload(25);
        std::cout << "Synthetic Matrix Workload Trace (25x25): " << traceResult << "\n";
        std::cout << "Total Internal Audit Logs Recorded     : " << library.getTotalLogs() << "\n";
        std::cout << "Simulation Completed Successfully.\n";

    } catch (const SystemException& sysEx) {
        std::cerr << "System Error [" << sysEx.getErrorCode() << "]: " << sysEx.what() << "\n";
        return 1;
    } catch (const std::exception& ex) {
        std::cerr << "Unhandled Standard Exception: " << ex.what() << "\n";
        return 2;
    }

    return 0;
}
