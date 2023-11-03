#include <concurrent_unordered_map.h>
#include "../../common.h"
#include "NiMain/NiNode.h"
#include "Setting.h"
#include "BGSDistantTreeBlock.h"

AutoPtr(uintptr_t, qword_141EE43A8, 0x1EE43A8);
DefineIniSetting(bEnableStippleFade, Display);

concurrency::concurrent_unordered_map<uint32_t, TESObjectREFR*> InstanceFormCache;

void BGSDistantTreeBlock::InvalidateCachedForm(uint32_t FormId)
{
	InstanceFormCache.unsafe_erase(FormId & 0x00FFFFFF);
}

void BGSDistantTreeBlock::UpdateBlockVisibility(ResourceData *Data)
{
	ZoneScopedN("BGSDistantTreeBlock::UpdateBlockVisibility");

	for (uint32_t i = 0; i < Data->m_LODGroups.QSize(); i++)
	{
		LODGroup *group = Data->m_LODGroups[i];

		for (uint32_t j = 0; j < group->m_LODInstances.QSize(); j++)
		{
			LODGroupInstance *instance = &group->m_LODInstances[j];
			const uint32_t maskedFormId = instance->FormId & 0x00FFFFFF;

			// Check if this instance was cached, otherwise search each plugin
			concurrency::concurrent_unordered_map<uint32_t, TESObjectREFR*>::iterator iterator;
			TESObjectREFR* treeReference = nullptr;
			iterator = InstanceFormCache.find(maskedFormId);
			if (iterator != InstanceFormCache.end())
			{
				treeReference = iterator->second;
			}
			else
			{
				// Find first valid tree object by ESP/ESM load order (TESDataHandler::Singleton()->PluginCount)
				for (uint32_t k = 0; k < *(uint32_t *)(qword_141EE43A8 + 0xD80); k++)
				{
					TESForm *form = TESForm::LookupFormById((k << 24) | maskedFormId);

					//
					// This has a few requirements...the form must:
					// - Be Loaded
					// - Be TESObjectREFR
					// - Have a base object that is TESObjectTREE or have a flag set (0x40)
					//
					if (!form)
						continue;

					TESObjectREFR *ref = form->IsREFR();

					if (!ref)
						continue;

					TESForm *baseForm = ref->GetBaseObject();

					if (baseForm)
					{
						if ((*(uint32_t*)((__int64)baseForm + 16) >> 6) & 1 || *(uint8_t *)((__int64)baseForm + 0x1A) == 38)
							treeReference = ref;

						if (treeReference)
							break;
					}
				}

				// Cache even if it's a null pointer
				InstanceFormCache.insert(std::make_pair(maskedFormId, treeReference));
			}

			bool fullyHidden = false;
			float alpha = 1.0f;

			if (treeReference)
			{
				NiNode *node = treeReference->GetNiNode();

				if (node && !node->QAppCulled() && treeReference->GetParentCell()->IsAttached())
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

				if (*(uint32_t *)((__int64)treeReference + 16) & (0x800 | 0x20))// IsDisabled | IsDeleted
					fullyHidden = true;
			}

			AutoFunc(uint16_t(__fastcall *)(float), Float2Half, 0xD41D80);
			uint16_t halfFloat = Float2Half(alpha);

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