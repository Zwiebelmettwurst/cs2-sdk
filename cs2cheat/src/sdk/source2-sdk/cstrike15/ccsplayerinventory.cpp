#include "ccsplayerinventory.hpp"

#include "../../memory/memory.hpp"

#include "../gcsdk/cgcclientsharedobjecttypecache.hpp"

static CGCClientSharedObjectTypeCache* CreateBaseTypeCache(
    CCSPlayerInventory* pInventory) {
    CGCClientSystem* pGCClientSystem = CGCClientSystem::GetInstance();
    if (!pGCClientSystem) return nullptr;

    CGCClient* pGCClient = pGCClientSystem->GetCGCClient();
    if (!pGCClient) return nullptr;

    CGCClientSharedObjectCache* pSOCache =
        pGCClient->FindSOCache(pInventory->GetOwnerID());
    if (!pSOCache) return nullptr;

    return pSOCache->CreateBaseTypeCache(k_EEconTypeItem);
}

CCSPlayerInventory* CCSPlayerInventory::GetInstance() {
    CCSInventoryManager* pInventoryManager = CCSInventoryManager::GetInstance();
    if (!pInventoryManager) return nullptr;

    return pInventoryManager->GetLocalInventory();
}

bool CCSPlayerInventory::AddEconItem(CEconItem* pItem) {
    // Helper function to aid in adding items.
    if (!pItem) return false;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (!pSOTypeCache || !pSOTypeCache->AddObject((CSharedObject*)pItem))
        return false;

    SOCreated(GetOwnerID(), (CSharedObject*)pItem, eSOCacheEvent_Incremental);
    return true;
}

void CCSPlayerInventory::RemoveEconItem(CEconItem* pItem) {
    // Helper function to aid in removing items.
    if (!pItem) return;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (!pSOTypeCache) return;

    const CUtlVector<CEconItem*>& pSharedObjects =
        pSOTypeCache->GetVecObjects<CEconItem*>();
    if (!pSharedObjects.Exists(pItem)) return;

    SODestroyed(GetOwnerID(), (CSharedObject*)pItem, eSOCacheEvent_Incremental);
    pSOTypeCache->RemoveObject((CSharedObject*)pItem);

    pItem->Destruct();
}

std::pair<uint64_t, uint32_t> CCSPlayerInventory::GetHighestIDs() {
    uint64_t maxItemID = 0;
    uint32_t maxInventoryID = 0;

    CGCClientSharedObjectTypeCache* pSOTypeCache = ::CreateBaseTypeCache(this);
    if (pSOTypeCache) {
        const CUtlVector<CEconItem*>& vecItems =
            pSOTypeCache->GetVecObjects<CEconItem*>();

        for (CEconItem* pEconItem : vecItems) {
            if (!pEconItem) continue;

            // Checks if item is default.
            if ((pEconItem->m_ulID & 0xF000000000000000) != 0) continue;

            maxItemID = std::max(maxItemID, pEconItem->m_ulID);
            maxInventoryID = std::max(maxInventoryID, pEconItem->m_unInventory);
        }
    }

    return std::make_pair(maxItemID, maxInventoryID);
}

C_EconItemView* CCSPlayerInventory::GetEconItemViewByItemID(uint64_t itemID) {
    C_EconItemView* pEconItemView = nullptr;

    const CUtlVector<C_EconItemView*>& pItems = GetVecInventoryItems();
    for (C_EconItemView* i : pItems) {
        if (i && i->m_iItemID() == itemID) {
            pEconItemView = i;
            break;
        }
    }

    return pEconItemView;
}
