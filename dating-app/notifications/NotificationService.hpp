#ifndef DATING_NOTIFICATION_SERVICE_HPP
#define DATING_NOTIFICATION_SERVICE_HPP

#include <algorithm>
#include <cstddef>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "../errors/AppError.hpp"
#include "NotificationChannel.hpp"

// The subject. One user, any number of channels.
//
// Original:
//     std::map<std::string, NotificationObserver*> observers;
//     observers[userId] = observer;          // <- assignment, not push_back
//
// That map holds one pointer per key, so the second channel for a user replaced
// the first. A user with email and push only ever got push, and the email
// observer was gone without a word.
//
// Here the value is a vector, and subscribe() appends. Adding a channel is
// additive by construction, which is the whole point: tomorrow's WhatsApp or
// Telegram channel is one more class and one more subscribe call, and the
// existing deliveries keep working.
class NotificationService {
public:
    void subscribe(const std::string &userId, std::unique_ptr<NotificationChannel> channel) {
        if (userId.empty()) {
            throw std::invalid_argument("NotificationService: user id must not be empty");
        }
        if (channel == nullptr) {
            throw std::invalid_argument("NotificationService: channel must not be null");
        }
        std::vector<std::unique_ptr<NotificationChannel>> &channels = channels_[userId];
        for (const std::unique_ptr<NotificationChannel> &existing : channels) {
            if (existing->name() == channel->name()) {
                throw std::invalid_argument("NotificationService: user " + userId
                                            + " already has a " + channel->name() + " channel");
            }
        }
        channels.push_back(std::move(channel));
    }

    // Removes one kind of channel and leaves every other one registered.
    // Returns how many were removed (always 1 here, or it throws).
    std::size_t unsubscribe(const std::string &userId, const std::string &channelName) {
        const auto found = channels_.find(userId);
        if (found == channels_.end()) {
            throw AppError("user " + userId + " has no notification channels");
        }
        std::vector<std::unique_ptr<NotificationChannel>> &channels = found->second;
        const std::size_t before = channels.size();
        channels.erase(std::remove_if(channels.begin(), channels.end(),
                                      [&channelName](const std::unique_ptr<NotificationChannel> &channel) {
                                          return channel->name() == channelName;
                                      }),
                       channels.end());
        const std::size_t removed = before - channels.size();
        if (removed == 0) {
            throw AppError("user " + userId + " has no " + channelName + " channel");
        }
        if (channels.empty()) {
            channels_.erase(found);
        }
        return removed;
    }

    // Delivers to every channel this user has. Returns the number of deliveries.
    // A user with no channels is not an error - they simply hear nothing.
    std::size_t notify(const std::string &userId, const std::string &message) const {
        const auto found = channels_.find(userId);
        if (found == channels_.end()) {
            return 0;
        }
        for (const std::unique_ptr<NotificationChannel> &channel : found->second) {
            channel->deliver(userId, message);
        }
        return found->second.size();
    }

    std::size_t notifyAll(const std::string &message) const {
        std::size_t delivered = 0;
        for (const auto &entry : channels_) {
            delivered += notify(entry.first, message);
        }
        return delivered;
    }

    std::size_t channelCount(const std::string &userId) const {
        const auto found = channels_.find(userId);
        return found == channels_.end() ? 0 : found->second.size();
    }

    std::size_t subscriberCount() const {
        return channels_.size();
    }

    std::size_t channelTotal() const {
        std::size_t total = 0;
        for (const auto &entry : channels_) {
            total += entry.second.size();
        }
        return total;
    }

    std::vector<std::string> channelNames(const std::string &userId) const {
        std::vector<std::string> names;
        const auto found = channels_.find(userId);
        if (found == channels_.end()) {
            return names;
        }
        for (const std::unique_ptr<NotificationChannel> &channel : found->second) {
            names.push_back(channel->name());
        }
        return names;
    }

private:
    // std::map keeps users in id order, so notifyAll() and the demo output are
    // deterministic. The inner vector keeps registration order.
    std::map<std::string, std::vector<std::unique_ptr<NotificationChannel>>> channels_;
};

#endif // DATING_NOTIFICATION_SERVICE_HPP
