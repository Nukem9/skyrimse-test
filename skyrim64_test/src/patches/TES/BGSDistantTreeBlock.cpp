#include "../../../tbb2018/concurrent_hash_map.h"
#include "../../common.h"
#include "NiMain/NiNode.h"
#include "Setting.h"
#include "BGSDistantTreeBlock.h"

DefineIniSetting(bEnableStippleFade, Display);

tbb::concurrent_hash_map<uint32_t, TESObjectTREE *> InstanceFormCache;

void BGSDistantTreeBlock::InvalidateCachedForm(uint32_t FormId)
{
	InstanceFormCache.erase(FormId & 0x00FFFFFF);
}

void BGSDistantTreeBlock::UpdateLODAlphaFade(ResourceData *Data)
{
	for (uint32_t i = 0; i < Data->m_LODGroups.QSize(); i++)
	{
		LODGroup *group = Data->m_LODGroups[i];

		for (uint32_t j = 0; j < group->m_LODInstances.QSize(); j++)
		{
			LODGroupInstance *instance = &group->m_LODInstances[j];
			const uint32_t maskedFormId = instance->FormId & 0x00FFFFFF;

			// Check if this instance was cached, otherwise search each plugin
			tbb::concurrent_hash_map<uint32_t, TESObjectTREE *>::accessor accessor;
			TESObjectTREE *treeObject = nullptr;

			if (InstanceFormCache.find(accessor, maskedFormId))
			{
				treeObject = accessor->second;
			}
			else
			{
				// Find first valid tree object by ESP/ESM load order
				for (int k = 0; k < TES_FORM_MASTER_COUNT; k++)
				{
					TESForm *form = TESForm::LookupFormById((k << 24) | maskedFormId);
					TESObjectREFR *ref = nullptr;

					if (form)
						ref = form->IsREFR();

					if (ref)
					{
						__int64 unkPtr = *(__int64 *)((__int64)ref + 0x40);

						if (unkPtr)
						{
							// Checks if the form type is TREE (TESObjectTREE) and some other flag 0x40
							if ((*(uint32_t *)((__int64)unkPtr + 16) >> 6) & 1 || *(uint8_t *)((__int64)unkPtr + 0x1A) == 38)
								treeObject = static_cast<TESObjectTREE *>(ref);
						}
					}

					if (treeObject)
						break;
				}

				// Insert even if it's a null pointer
				InstanceFormCache.insert(std::make_pair(maskedFormId, treeObject));
			}

			bool fullyHidden = false;
			float alpha = 1.0f;

			if (treeObject)
			{
				NiNode *node = treeObject->GetNiNode();
				__int64 cell = *(__int64 *)((__int64)treeObject + 0x60);// TESObjectREFR::GetParentCell()?

				if (node && !node->GetAppCulled() && *(uint8_t *)(cell + 0x44) == 7)
				{
					if (bEnableStippleFade->uValue.b)
					{
						void *fadeNode = node->IsFadeNode();

						if (fadeNode)
						{
							alpha = 1.0f - *(float *)((__int64)fadeNode + 0x130);// BSFadeNode::fCurrentFade

							if (alpha <= 0.0f)
								fullyHidden = true;
						}
					}
					else
					{
						// No alpha fade - LOD trees will instantly appear or disappear
						fullyHidden = true;
					}
				}

				if (*(uint32_t *)((__int64)treeObject + 16) & 0x820)
					fullyHidden = true;
			}

			uint16_t halfFloat = ((uint16_t(__fastcall *)(float))(g_ModuleBase + 0xD41D80))(alpha);
			//uint16_t halfFloat = Float2Half(alpha);

			if (instance->Alpha != halfFloat)
			{
				instance->Alpha = halfFloat;
				group->m_UnkByte24 = false;
			}

			if (instance->Hidden != fullyHidden)
			{
				instance->Hidden = fullyHidden;
				group->m_UnkByte24 = false;
			}

			if (fullyHidden)
				Data->m_UnkByte82 = false;
		}
	}
}