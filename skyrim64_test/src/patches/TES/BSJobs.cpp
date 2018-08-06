#include <map>
#include <mutex>
#include "../../common.h"
#include "BSJobs.h"

const std::unordered_map<uintptr_t, std::string> BSJobs::JobNameMap =
{
	{ 0x12EDC30, "BSAccumProcess::DoSceneListAccumCullingJob", },
	{ 0x12EDC40, "BSAccumProcess::DoSceneListAccumRegisterJob", },
	{ 0x12F98E0, "FirstListAccumulationJob", },
	{ 0x12F9A70, "ListAccumulationJob", },
	{ 0x1280DC0, "VMProcess", },
	{ 0x10E0780, "PathingTaskData::ProcessPhysicsPath", },
	{ 0x7A7F10,  "CombatUpdateJob", },
	{ 0x754CA0,  "ProjectileJobs::ProjectileJob", },
	{ 0x6E14B0,  "ProcessListJobs::ProcessActorAnimSG", },
	{ 0x6E14D0,  "ProcessListJobs::UpdateRagdollPostPhysics", },
	{ 0x6E18A0,  "ProcessListJobs::RunOneActorUpdateSkyCellSkinJob", },
	{ 0x6E17D0,  "ProcessListJobs::RunOneActorAnimationUpdateJob", },
	{ 0x6E14F0,  "ProcessListJobs::UpdateActor", },
	{ 0x6E13F0,  "ProcessListJobs::UpdateActorMovementJob", },
	{ 0x6E1450,  "ProcessListJobs::UpdatePlayerMovementJob", },
	{ 0x640360,  "UpdateNonHighActors: Update low list helper", },
	{ 0x5C8F20,  "UpdateCellNode", },
	{ 0x5B8CD0,  "ClearListJobFunc", },
	{ 0x5B7AD0,  "DrawWorld_BuildSceneLists", },
	{ 0x3D9AB0,  "MorphingJobList::UpdateMorphingJob", },
	{ 0x37B3C0,  "UpdateQuestJob", },
	{ 0x2720B0,  "CELLJobs::UpdateParticleSystemManagerJob", },
	{ 0x2720A0,  "CELLJobs::UpdateQueuedParticlesJob", },
	{ 0x271FB0,  "CELLJobs::UpdateAnimationJob", },
	{ 0x25F340,  "TESObjectCELL::PrepareUpdateAnimatedRefsJob", },
	{ 0x25F3C0,  "TESObjectCELL::UpdateAnimatedRefsJob", },
	{ 0x25F2D0,  "TESObjectCELL::UpdateManagedNodesJob", },
	{ 0x25F320,  "TESObjectCELL::Pre_UpdateManagedNodesJob", },
	{ 0x25F290,  "TESObjectCELL::UpdateExteriorWorldJob", },
	{ 0xD511E0,  "CullingJobList::Job", },

	{ 0x63FB90, "TES animation" },
	{ 0x63FAC0, "Water audio" },
	{ 0x63FAE0, "Texture update" },
	{ 0x63FB60, "Combat manager" },
	{ 0x640F10, "Partial Terrain Manager Update" },
	{ 0x63FC10, "JobListEnd" },
	{ 0x63FCB0, "Actor update" },
	{ 0x63FC80, "Wait Player Update 1" },
	{ 0x63FD50, "JobListEnd" },

	{ 0x63FD30, "Actor update end" },
	{ 0x63FD70, "Actor movement" },
	{ 0x575100, "Actor movement signal/sync" },
	{ 0x63FEF0, "Player animation" },
	{ 0x5750E0, "Player Anim signal" },
	{ 0x640000, "Animation files" },
	{ 0x640020, "Movement avoid box" },
	{ 0x63FF20, "Detection" },
	{ 0x63FF50, "Pick reference" },
	{ 0x640040, "Auto aim" },
	{ 0x5750F0, "Player Anim sync" },
	{ 0x63FDE0, "Actor animation" },
	{ 0x575100, "Actors Update SigSync" },
	{ 0x640060, "Ragdoll animation" },
	{ 0x640440, "VM update" },
	{ 0x575100, "Ragdoll signal/sync" },
	{ 0x640E90, "ClonePools Update" },
	{ 0x640090, "Combat and magic" },
	{ 0x63FF40, "Destructible" },
	{ 0x640120, "JobListEnd" },

	{ 0x63FF90, "Quest for events" },
	{ 0x575100, "Quests/Scenes update sync" },
	{ 0x63FAF0, "UI" },
	{ 0x640340, "Process lists" },
	{ 0x640360, "Update low list helper 1" },
	{ 0x640360, "Update low list helper 2" },
	{ 0x640360, "Update low list helper 3" },
	{ 0x640780, "Script clear" },
	{ 0x63FF70, "Wait For Shadow Culling" },
	{ 0x575100, "Shadow Culling sync" },
	{ 0x640140, "Cell animations" },
	{ 0x640210, "Ragdoll post physics" },
	{ 0x575100, "World update  sync" },
	{ 0x6403E0, "Reset Low list helper" },
	{ 0x6403A0, "Low messages" },
	{ 0x640400, "VM render-safe" },
	{ 0x6407B0, "IO" },
	{ 0x6405E0, "JobListEnd" },

	{ 0x640610, "Garbage" },
	{ 0x575100, "Garbage signal/sync" },
	{ 0x640640, "Ref movement" },
	{ 0x640660, "Path update" },
	{ 0x6406C0, "Bounds manager" },
	{ 0x640720, "Avoidance manager" },
	{ 0x6407A0, "Sleep manager" },
	{ 0x575100, "AI signal/sync" },
	{ 0x640880, "Havok reset" },
	{ 0x575100, "Havok signal/sync" },
	{ 0x6408B0, "Post swap" },
	{ 0x6408F0, "Particles" },
	{ 0x640B60, "Pathing and nav-mesh" },
	{ 0x640CD0, "JobListEnd" },

	{ 0x640960, "Non render-safe AI" },
	{ 0x575100, "Anim SG signal/sync" },
	{ 0x640E00, "Animated cell object update" },
	{ 0x575100, "Cell update signal/sync" },
	{ 0x640BF0, "Post process" },
	{ 0x640BD0, "Poll controls" },
	{ 0x640CF0, "JobListEnd" },

	{ 0x640D40, "Face morphing" },
	{ 0x6408D0, "Sky" },
	{ 0x640DB0, "Shared particles" },
	{ 0x640DC0, "Update grass" },
	{ 0x575100, "End signal/sync" },
	{ 0x640DE0, "Post reset" },

	// Not sure why this one is separate from the rest
	{ 0xC33790, "JobListEnd" },
};

std::unordered_map<uintptr_t, BSJobs::TrackingInfo> BSJobs::JobTracker;
#if SKYRIM64_USE_TRACY
std::unordered_map<uintptr_t, tracy::SourceLocationData> TracySourceMap;
#endif

void BSJobs::DispatchJobCallback(void *Parameter, void(*Function)(void *))
{
	// Populate the static tables once - abuse thread safe statics to call a function instead
	static int threadSafeInit = []() -> int
	{
		for (auto& nameEntry : BSJobs::JobNameMap)
		{
#if SKYRIM64_USE_TRACY
			tracy::SourceLocationData srcLocation{ nameEntry.second.c_str(), nameEntry.second.c_str(), "<unknown>", 0, 0 };
			TracySourceMap.insert_or_assign(nameEntry.first, srcLocation);
#endif

			// These only need to be initialized to zero (hacky code here)
			BSJobs::JobTracker[nameEntry.first] = {};

			auto& ref = BSJobs::JobTracker[nameEntry.first];
			ref.Name = nameEntry.second;
			ref.TotalCount.store(0);
			ref.ActiveCount.store(0);
		}

		return 0;
	}();

	// Now do actual work
	uintptr_t offset = (uintptr_t)Function - g_ModuleBase;
	auto counterEntry = JobTracker.find(offset);

	AssertMsgVa(counterEntry != JobTracker.end(), "Unknown job callback 0x%p", offset);

	counterEntry->second.TotalCount++;
	counterEntry->second.ActiveCount++;

#if SKYRIM64_USE_TRACY
	tracy::ScopedZone ___tracy_scoped_zone(&TracySourceMap[offset]);
#endif

	Function(Parameter);
}