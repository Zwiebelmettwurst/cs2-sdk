#include <vector>

#include "skin_changer.hpp"

#include "../../sdk/interfaces/interfaces.hpp"

static int g_lastEquippedCount;
static std::vector<uint64_t> g_vecAddedItemsIDs;

void skin_changer::OnFrameStageNotify(int frameStage) {
    if (frameStage != 6 || !interfaces::pEngine->IsInGame()) return;

    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
    if (!pInventory) return;

    CGameEntitySystem* pEntitySystem = CGameEntitySystem::GetInstance();
    if (!pEntitySystem) return;

    const uint64_t steamID = pInventory->GetOwnerID().m_id;

    CCSPlayerController* pLocalPlayerController =
        CGameEntitySystem::GetLocalPlayerController();
    if (!pLocalPlayerController) return;

    C_CSPlayerPawn* pLocalPawn =
        pLocalPlayerController->m_hPawn().Get<C_CSPlayerPawn>();
    if (!pLocalPawn) return;

    CPlayer_WeaponServices* pWeaponServices = pLocalPawn->m_pWeaponServices();
    if (!pWeaponServices) return;

    CHandle hActiveWeapon = pWeaponServices->m_hActiveWeapon();

    CCSPlayer_ViewModelServices* pViewModelServices =
        pLocalPawn->m_pViewModelServices();
    if (!pViewModelServices) return;

    C_BaseViewModel* pViewModel =
        pViewModelServices->m_hViewModel()[0].Get<C_BaseViewModel>();

    int highestIndex = pEntitySystem->GetHighestEntityIndex();
    for (int i = MAX_PLAYERS + 1; i <= highestIndex; ++i) {
        C_BaseEntity* pEntity = pEntitySystem->GetBaseEntity(i);
        if (!pEntity || !pEntity->IsBasePlayerWeapon()) continue;

        C_WeaponCSBase* pWeapon = reinterpret_cast<C_WeaponCSBase*>(pEntity);
        if (pWeapon->GetOriginalOwnerXuid() != steamID) continue;

        C_AttributeContainer* pAttributeContainer =
            pWeapon->m_AttributeManager();
        if (!pAttributeContainer) continue;

        C_EconItemView* pWeaponItemView = pAttributeContainer->m_Item();
        if (!pWeaponItemView) continue;

        CEconItemDefinition* pWeaponDefinition =
            pWeaponItemView->GetStaticData();
        if (!pWeaponDefinition) continue;

        CGameSceneNode* pWeaponSceneNode = pWeapon->m_pGameSceneNode();
        if (!pWeaponSceneNode) continue;

        C_EconItemView* pWeaponInLoadoutItemView =
            pInventory->GetItemInLoadout(pWeapon->m_iOriginalTeamNumber(),
                                         pWeaponDefinition->GetLoadoutSlot());
        if (!pWeaponInLoadoutItemView) continue;

        // Check if skin is added by us.
        auto it =
            std::find(g_vecAddedItemsIDs.cbegin(), g_vecAddedItemsIDs.cend(),
                      pWeaponInLoadoutItemView->m_iItemID());
        if (it == g_vecAddedItemsIDs.cend()) continue;

        CEconItemDefinition* pWeaponInLoadoutDefinition =
            pWeaponInLoadoutItemView->GetStaticData();
        if (!pWeaponInLoadoutDefinition) continue;

        // Example: Will not equip FiveSeven skin on CZ.
        const bool isKnife = pWeaponInLoadoutDefinition->IsKnife(false);
        if (!isKnife && pWeaponInLoadoutDefinition->GetDefinitionIndex() !=
                            pWeaponDefinition->GetDefinitionIndex())
            continue;

        pWeaponItemView->m_iItemID() = pWeaponInLoadoutItemView->m_iItemID();
        pWeaponItemView->m_iItemIDHigh() =
            pWeaponInLoadoutItemView->m_iItemIDHigh();
        pWeaponItemView->m_iItemIDLow() =
            pWeaponInLoadoutItemView->m_iItemIDLow();
        pWeaponItemView->m_iAccountID() = uint32_t(steamID);

        CHandle hWeapon = pWeapon->GetRefEHandle();
        if (isKnife) {
            pWeaponItemView->m_iItemDefinitionIndex() =
                pWeaponInLoadoutDefinition->GetDefinitionIndex();

            const char* knifeModel = pWeaponInLoadoutDefinition->GetModelName();
            pWeapon->SetModel(knifeModel);
            if (hWeapon == hActiveWeapon && pViewModel)
                pViewModel->SetModel(knifeModel);
        } else {
            // Workaround: We are forcing the OLD Models.
            pWeaponSceneNode->SetMeshGroupMask(2);
            if (hWeapon == hActiveWeapon && pViewModel) {
                CGameSceneNode* pViewModelSceneNode =
                    pViewModel->m_pGameSceneNode();

                if (pViewModelSceneNode)
                    pViewModelSceneNode->SetMeshGroupMask(2);
            }
        }
    }
}

void skin_changer::OnPreFireEvent(CGameEvent* pEvent) {
    if (!pEvent) return;

    const char* eventName = pEvent->GetName();
    if (!eventName) return;

    static constexpr auto player_death = hash_32_fnv1a_const("player_death");
    if (hash_32_fnv1a_const(eventName) != player_death) return;

    CCSPlayerController* pControllerWhoKilled =
        pEvent->GetPlayerController("attacker");
    CCSPlayerController* pControllerWhoDied =
        pEvent->GetPlayerController("userid");
    if (pControllerWhoKilled == pControllerWhoDied) return;

    CCSPlayerController* pLocalPlayerController =
        CGameEntitySystem::GetLocalPlayerController();
    if (!pLocalPlayerController ||
        pControllerWhoKilled != pLocalPlayerController)
        return;

    C_CSPlayerPawn* pLocalPawn =
        pLocalPlayerController->m_hPawn().Get<C_CSPlayerPawn>();
    if (!pLocalPawn) return;

    CPlayer_WeaponServices* pWeaponServices = pLocalPawn->m_pWeaponServices();
    if (!pWeaponServices) return;

    C_WeaponCSBase* pActiveWeapon =
        pWeaponServices->m_hActiveWeapon().Get<C_WeaponCSBase>();
    if (!pActiveWeapon) return;

    C_AttributeContainer* pAttributeContainer =
        pActiveWeapon->m_AttributeManager();
    if (!pAttributeContainer) return;

    C_EconItemView* pWeaponItemView = pAttributeContainer->m_Item();
    if (!pWeaponItemView) return;

    CEconItemDefinition* pWeaponDefinition = pWeaponItemView->GetStaticData();
    if (!pWeaponDefinition || !pWeaponDefinition->IsKnife(true)) return;

    pEvent->SetString("weapon", pWeaponDefinition->GetSimpleWeaponName());
}

void skin_changer::OnSoUpdated(
    CEconDefaultEquippedDefinitionInstanceClient* pObject) {
    if (g_lastEquippedCount <= 0 ||
        pObject->GetTypeID() !=
            k_EEconTypeDefaultEquippedDefinitionInstanceClient)
        return;

    pObject->GetDefinitionIndex() = 0;
    --g_lastEquippedCount;
}

void skin_changer::OnEquipItemInLoadout(int team, int slot, uint64_t itemID) {
    auto it =
        std::find(g_vecAddedItemsIDs.begin(), g_vecAddedItemsIDs.end(), itemID);
    if (it == g_vecAddedItemsIDs.end()) return;

    CCSInventoryManager* pInventoryManager = CCSInventoryManager::GetInstance();
    if (!pInventoryManager) return;

    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
    if (!pInventory) return;

    C_EconItemView* pItemViewToEquip = pInventory->GetEconItemViewByItemID(*it);
    if (!pItemViewToEquip) return;

    C_EconItemView* pItemInLoadout = pInventory->GetItemInLoadout(team, slot);
    if (!pItemInLoadout) return;

    CEconItemDefinition* pItemInLoadoutStaticData =
        pItemInLoadout->GetStaticData();
    if (!pItemInLoadoutStaticData) return;

    if (pItemInLoadoutStaticData->IsGlove(false) ||
        pItemInLoadoutStaticData->IsKnife(false) ||
        pItemInLoadoutStaticData->GetDefinitionIndex() ==
            pItemViewToEquip->m_iItemDefinitionIndex())
        return;

    // Equip default item. If you would have bought Deagle and you previously
    // had R8 equipped it will now give you a Deagle.
    const uint64_t defaultItemID =
        (std::uint64_t(0xF) << 60) | pItemViewToEquip->m_iItemDefinitionIndex();
    pInventoryManager->EquipItemInLoadout(team, slot, defaultItemID);
    ++g_lastEquippedCount;

    CEconItem* pItemInLoadoutSOCData = pItemInLoadout->GetSOCData();
    if (!pItemInLoadoutSOCData) return;

    // Mark old item as unequipped.
    pInventory->SOUpdated(pInventory->GetOwnerID(),
                          (CSharedObject*)pItemInLoadoutSOCData,
                          eSOCacheEvent_Incremental);
}

void skin_changer::OnSetModel(C_BaseModelEntity* pEntity, const char*& model) {
    // When you're lagging you may see the default knife for one second and this
    // function fixes that.
    if (!pEntity || !pEntity->IsViewModel()) return;

    C_BaseViewModel* pViewModel = (C_BaseViewModel*)pEntity;

    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
    if (!pInventory) return;

    const uint64_t steamID = pInventory->GetOwnerID().m_id;

    C_WeaponCSBase* pWeapon = pViewModel->m_hWeapon().Get<C_WeaponCSBase>();
    if (!pWeapon || !pWeapon->IsBasePlayerWeapon() ||
        pWeapon->GetOriginalOwnerXuid() != steamID)
        return;

    C_AttributeContainer* pAttributeContainer = pWeapon->m_AttributeManager();
    if (!pAttributeContainer) return;

    C_EconItemView* pWeaponItemView = pAttributeContainer->m_Item();
    if (!pWeaponItemView) return;

    CEconItemDefinition* pWeaponDefinition = pWeaponItemView->GetStaticData();
    if (!pWeaponDefinition) return;

    C_EconItemView* pWeaponInLoadoutItemView = pInventory->GetItemInLoadout(
        pWeapon->m_iOriginalTeamNumber(), pWeaponDefinition->GetLoadoutSlot());
    if (!pWeaponInLoadoutItemView) return;

    // Check if skin is added by us.
    auto it = std::find(g_vecAddedItemsIDs.cbegin(), g_vecAddedItemsIDs.cend(),
                        pWeaponInLoadoutItemView->m_iItemID());
    if (it == g_vecAddedItemsIDs.cend()) return;

    CEconItemDefinition* pWeaponInLoadoutDefinition =
        pWeaponInLoadoutItemView->GetStaticData();
    if (!pWeaponInLoadoutDefinition ||
        !pWeaponInLoadoutDefinition->IsKnife(true))
        return;

    model = pWeaponInLoadoutDefinition->GetModelName();
}

void skin_changer::AddEconItemToList(CEconItem* pItem) {
    g_vecAddedItemsIDs.emplace_back(pItem->m_ulID);
}

void skin_changer::Shutdown() {
    CCSPlayerInventory* pInventory = CCSPlayerInventory::GetInstance();
    if (!pInventory) return;

    for (uint64_t id : g_vecAddedItemsIDs) {
        C_EconItemView* pItemView = pInventory->GetEconItemViewByItemID(id);
        if (!pItemView) continue;

        pInventory->RemoveEconItem(pItemView->GetSOCData());
    }
}
