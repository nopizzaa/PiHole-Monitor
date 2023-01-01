#pragma once

typedef enum {
    PIHOLE_ENABLED,
    PIHOLE_DISABLED,
    PIHOLE_UNAVAILABLE
} PiHoleStatus;

typedef struct {
    boolean isPresent;
    String message;
} PiHoleQuerryError;

typedef struct {
    String clientAddress;
    int blockedCount;
} ClientBlocked;

typedef struct {
    String domains_being_blocked;
    String ads_blocked_today;
    String clients_ever_seen;
    String unique_clients;
    String ads_percentage_today;
    String piHoleStatus;
} PiholeDataStruct;