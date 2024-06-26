#include "pch-il2cpp.h"
#include "host_tab.h"
#include "utility.h"
#include "game.h"
#include "state.hpp"
#include "gui-helpers.hpp"
#include <algorithm>

namespace HostTab {
	static void SetRoleAmount(RoleTypes__Enum type, int amount) {
		auto&& options = GameOptions().GetRoleOptions();
		auto maxCount = options.GetNumPerGame(type);
		if (amount > maxCount)
			options.SetRoleRate(type, amount, 100);
		else if (amount > 0)
			options.SetRoleRate(type, maxCount, 100);
	}

	const ptrdiff_t GetRoleCount(RoleType role)
	{
		return std::count_if(State.assignedRoles.cbegin(), State.assignedRoles.cend(), [role](RoleType i) {return i == role; });
	}

	void Render() {
		if (IsHost()) {
			GameOptions options;
			if (IsInLobby()) {
				ImGui::SameLine(100 * State.dpiScale);
				ImGui::BeginChild("host#list", ImVec2(200, 0) * State.dpiScale, true, ImGuiWindowFlags_NoBackground);
				bool shouldEndListBox = ImGui::ListBoxHeader("Choose Roles", ImVec2(200, 150) * State.dpiScale);
				auto allPlayers = GetAllPlayerData();
				auto playerAmount = allPlayers.size();
				auto maxImpostorAmount = GetMaxImpostorAmount((int)playerAmount);
				for (size_t index = 0; index < playerAmount; index++) {
					auto playerData = allPlayers[index];
					if (playerData == nullptr) continue;
					PlayerControl* playerCtrl = GetPlayerControlById(playerData->fields.PlayerId);
					if (playerCtrl == nullptr) continue;
					State.assignedRolesPlayer[index] = playerCtrl;
					if (State.assignedRolesPlayer[index] == nullptr)
						continue;

					app::GameData_PlayerOutfit* outfit = GetPlayerOutfit(playerData);
					if (outfit == NULL) continue;
					const std::string& playerName = convert_from_string(GameData_PlayerOutfit_get_PlayerName(outfit, nullptr));
					//player colors in host tab by gdjkhp (https://github.com/GDjkhp/AmongUsMenu/commit/53b017183bac503c546f198e2bc03539a338462c)
					if (CustomListBoxInt(playerName.c_str(), reinterpret_cast<int*>(&State.assignedRoles[index]), ROLE_NAMES, 80 * State.dpiScale, AmongUsColorToImVec4(GetPlayerColor(outfit->fields.ColorId))))
					{
						State.engineers_amount = (int)GetRoleCount(RoleType::Engineer);
						State.scientists_amount = (int)GetRoleCount(RoleType::Scientist);
						State.shapeshifters_amount = (int)GetRoleCount(RoleType::Shapeshifter);
						State.impostors_amount = (int)GetRoleCount(RoleType::Impostor);
						if (State.impostors_amount + State.shapeshifters_amount > maxImpostorAmount)
						{
							if (State.assignedRoles[index] == RoleType::Shapeshifter)
								State.assignedRoles[index] = RoleType::Random;
							else if (State.assignedRoles[index] == RoleType::Impostor)
								State.assignedRoles[index] = RoleType::Random;
							State.shapeshifters_amount = (int)GetRoleCount(RoleType::Shapeshifter);
							State.impostors_amount = (int)GetRoleCount(RoleType::Impostor);
							State.crewmates_amount = (int)GetRoleCount(RoleType::Crewmate);
						}

						if (State.assignedRoles[index] == RoleType::Engineer || State.assignedRoles[index] == RoleType::Scientist || State.assignedRoles[index] == RoleType::Crewmate) {
							if (State.engineers_amount + State.scientists_amount + State.crewmates_amount >= (int)playerAmount)
								State.assignedRoles[index] = RoleType::Random;
						} //Some may set all players to non imps. This hangs the game on beginning. Leave space to Random so we have imps.

						if (!IsInGame()) {
							if (options.GetGameMode() == GameModes__Enum::HideNSeek)
								SetRoleAmount(RoleTypes__Enum::Engineer, 15);
							else
								SetRoleAmount(RoleTypes__Enum::Engineer, State.engineers_amount);
							SetRoleAmount(RoleTypes__Enum::Scientist, State.scientists_amount);
							SetRoleAmount(RoleTypes__Enum::Shapeshifter, State.shapeshifters_amount);
							if (options.GetNumImpostors() <= State.impostors_amount + State.shapeshifters_amount)
								options.SetInt(app::Int32OptionNames__Enum::NumImpostors, State.impostors_amount + State.shapeshifters_amount);
						}
					}
				}
				if (shouldEndListBox)
					ImGui::ListBoxFooter();
				ImGui::EndChild();
			}
			ImGui::SameLine();
			ImGui::BeginChild("host#actions", ImVec2(300, 0) * State.dpiScale, true, ImGuiWindowFlags_NoBackground);

			// AU v2022.8.24 has been able to change maps in lobby.
			State.mapHostChoice = options.GetByte(app::ByteOptionNames__Enum::MapId);
			if (State.mapHostChoice > 3)
				State.mapHostChoice--;
			State.mapHostChoice = std::clamp(State.mapHostChoice, 0, (int)MAP_NAMES.size() - 1);
			if (IsInLobby() && CustomListBoxInt("Map", &State.mapHostChoice, MAP_NAMES, 75 * State.dpiScale)) {
				if (!IsInGame()) {
					// disable flip
					/*if (State.mapHostChoice == 3) {
						options.SetByte(app::ByteOptionNames__Enum::MapId, 0);
						State.FlipSkeld = true;
					}
					else {
						options.SetByte(app::ByteOptionNames__Enum::MapId, State.mapHostChoice);
						State.FlipSkeld = false;
					}*/
					auto id = State.mapHostChoice;
					if (id >= 3) id++;
					options.SetByte(app::ByteOptionNames__Enum::MapId, id);
				}
			}
			if (IsInLobby() && ToggleButton("Custom Impostor Amount", &State.CustomImpostorAmount))
				State.Save();
			State.ImpostorCount = std::clamp(State.ImpostorCount, 0, int(Game::MAX_PLAYERS));
			if (IsInLobby() && ImGui::InputInt("Impostor Count", &State.ImpostorCount))
				State.Save();
			/*int32_t maxPlayers = options.GetMaxPlayers();
			maxPlayers = std::clamp(maxPlayers, 0, int(Game::MAX_PLAYERS));
			if (IsInLobby() && ImGui::InputInt("Max Players", &maxPlayers))
				GameOptions().SetInt(app::Int32OptionNames__Enum::MaxPlayers, maxPlayers);*/ //support for more than 15 players

			/*if (IsInLobby() && ToggleButton("Flip Skeld", &State.FlipSkeld))
				State.Save();*/ //to be fixed later
			ImGui::Dummy(ImVec2(7, 7) * State.dpiScale);
			if (IsInLobby() && ImGui::Button("Force Start of Game"))
			{
				app::InnerNetClient_SendStartGame((InnerNetClient*)(*Game::pAmongUsClient), NULL);
			}
			if (ToggleButton("Disable Meetings", &State.DisableMeetings))
				State.Save();
			if (ToggleButton("Disable Sabotages", &State.DisableSabotages))
				State.Save();
			if (ToggleButton("Disable Kills", &State.DisableKills))
				State.Save();
			if (State.DisableKills) ImGui::Text("Note: Cheaters can still bypass this feature!");

			/*if (ToggleButton("Disable Specific RPC Call ID", &State.DisableCallId))
				State.Save();
			int callId = State.ToDisableCallId;
			if (ImGui::InputInt("ID to Disable", &callId)) {
				State.ToDisableCallId = (uint8_t)callId;
				State.Save();
			}*/

			if ((State.mapType == Settings::MapType::Airship) && IsInGame() && ImGui::Button("Switch Moving Platform Side"))
			{
				State.rpcQueue.push(new RpcUsePlatform());
			}

			if (State.InMeeting && ImGui::Button("End Meeting")) {
				State.rpcQueue.push(new RpcEndMeeting());
				State.InMeeting = false;
			}

			if (IsInMultiplayerGame() || IsInLobby()) { //lobby isn't possible in freeplay
				if (ToggleButton("Disable Game Ending", &State.NoGameEnd)) {
					State.Save();
				}

				if (IsInGame()) {
					CustomListBoxInt("Reason", &State.SelectedGameEndReasonId, GAMEENDREASON, 120.0f * State.dpiScale);

					ImGui::SameLine();

					if (ImGui::Button("End Game")) {
							State.rpcQueue.push(new RpcEndGame(GameOverReason__Enum(std::clamp(State.SelectedGameEndReasonId, 0, 8))));
					}
				}
			}

			CustomListBoxInt(" �", &State.HostSelectedColorId, HOSTCOLORS, 85.0f * State.dpiScale);

			if (ToggleButton("Force Color for Everyone", &State.ForceColorForEveryone)) {
				State.Save();
			}

			if (ToggleButton("Force Name for Everyone", &State.ForceNameForEveryone)) {
				State.Save();
			}
			if (InputString("Username", &State.hostUserName)) {
				State.Save();
			}

			// label "Lobby Settings"
			ImGui::Text("Lobby Settings");


			// PlayerSpeedMod
			if (ImGui::InputFloat("Player Speed", &State.PlayerSpeedMod)) {
				options.SetFloat(app::FloatOptionNames__Enum::PlayerSpeedMod, State.PlayerSpeedMod);
			}

			// Tasks amount
			bool commonTasksChanged = ImGui::InputInt("CommonTasks", &State.CommonTasks);
			bool shortTasksChanged = ImGui::InputInt("ShortTasks", &State.ShortTasks);
			bool longTasksChanged = ImGui::InputInt("LongTasks", &State.LongTasks);


			if (commonTasksChanged || shortTasksChanged || longTasksChanged) {
				// Ensure values are within individual limits first
				State.CommonTasks = std::clamp(State.CommonTasks, 0, 255);
				State.ShortTasks = std::clamp(State.ShortTasks, 0, 255);
				State.LongTasks = std::clamp(State.LongTasks, 0, 255);

				int totalTasks = State.CommonTasks + State.ShortTasks + State.LongTasks;
				if (totalTasks > 255) {
					int excess = totalTasks - 255;

					// Adjust the most recently changed value to fit the total within 255
					if (commonTasksChanged) {
						State.CommonTasks = max(0, State.CommonTasks - excess);
					}
					else if (shortTasksChanged) {
						State.ShortTasks = max(0, State.ShortTasks - excess);
					}
					else if (longTasksChanged) {
						State.LongTasks = max(0, State.LongTasks - excess);
					}

				}

				// Update values in options only if within the overall range
				if (commonTasksChanged) {
					options.SetInt(app::Int32OptionNames__Enum::NumCommonTasks, State.CommonTasks);
				}
				if (shortTasksChanged) {
					options.SetInt(app::Int32OptionNames__Enum::NumShortTasks, State.ShortTasks);
				}
				if (longTasksChanged) {
					options.SetInt(app::Int32OptionNames__Enum::NumLongTasks, State.LongTasks);
				}
			}

			State.PlayerSpeedMod = options.GetFloat(app::FloatOptionNames__Enum::PlayerSpeedMod);
			State.CommonTasks = options.GetInt(app::Int32OptionNames__Enum::NumCommonTasks);
			State.ShortTasks = options.GetInt(app::Int32OptionNames__Enum::NumShortTasks);
			State.LongTasks = options.GetInt(app::Int32OptionNames__Enum::NumLongTasks);

			if (options.GetGameMode() == GameModes__Enum::HideNSeek) {
				ImGui::Text("Hide N Seek Settings");
				if (ImGui::InputInt("Vent Uses", &State.CrewmateVentUses))
					options.SetInt(app::Int32OptionNames__Enum::CrewmateVentUses, State.CrewmateVentUses);
				State.CrewmateVentUses = options.GetInt(app::Int32OptionNames__Enum::CrewmateVentUses);
			}
			else {
				ImGui::Text("Roles Settings");
				if (ImGui::InputFloat("Kill CD", &State.KillCooldown))
					options.SetFloat(app::FloatOptionNames__Enum::KillCooldown, State.KillCooldown);
				if (ImGui::InputFloat("Vent CD", &State.EngineerCooldown))
					options.SetFloat(app::FloatOptionNames__Enum::EngineerCooldown, State.EngineerCooldown);
				if (ImGui::InputFloat("Vent time", &State.EngineerInVentMaxTime))
					options.SetFloat(app::FloatOptionNames__Enum::EngineerInVentMaxTime, State.EngineerInVentMaxTime);
				if (ImGui::InputFloat("Protect CD", &State.GuardianAngelCooldown))
					options.SetFloat(app::FloatOptionNames__Enum::GuardianAngelCooldown, State.GuardianAngelCooldown);
				if (ImGui::InputFloat("SS CD", &State.ShapeshifterCooldown))
					options.SetFloat(app::FloatOptionNames__Enum::ShapeshifterCooldown, State.ShapeshifterCooldown);
				if (ImGui::InputFloat("SS time", &State.ShapeshifterDuration))
					options.SetFloat(app::FloatOptionNames__Enum::ShapeshifterDuration, State.ShapeshifterDuration);



				if (ImGui::Button("0CD"))
				{
					State.KillCooldown = 1.4E-45f;
					State.EngineerCooldown = 1.4E-45f;
					State.GuardianAngelCooldown = 1.4E-45f;
					State.ShapeshifterCooldown = 1.4E-45f;
					options.SetFloat(app::FloatOptionNames__Enum::KillCooldown, State.KillCooldown);
					options.SetFloat(app::FloatOptionNames__Enum::EngineerCooldown, State.EngineerCooldown);
					options.SetFloat(app::FloatOptionNames__Enum::GuardianAngelCooldown, State.GuardianAngelCooldown);
					options.SetFloat(app::FloatOptionNames__Enum::ShapeshifterCooldown, State.ShapeshifterCooldown);
				}

				State.KillCooldown = options.GetFloat(app::FloatOptionNames__Enum::KillCooldown);
				State.EngineerCooldown = options.GetFloat(app::FloatOptionNames__Enum::EngineerCooldown);
				State.EngineerInVentMaxTime = options.GetFloat(app::FloatOptionNames__Enum::EngineerInVentMaxTime);
				State.GuardianAngelCooldown = options.GetFloat(app::FloatOptionNames__Enum::GuardianAngelCooldown);
				State.ShapeshifterCooldown = options.GetFloat(app::FloatOptionNames__Enum::ShapeshifterCooldown);
				State.ShapeshifterDuration = options.GetFloat(app::FloatOptionNames__Enum::ShapeshifterDuration);
			}




			static int framesPassed = -1;
			static bool isReviving = false;

			if (IsHost() && IsInGame() && !isReviving && GetPlayerData(*Game::pLocalPlayer)->fields.IsDead && ImGui::Button("Revive Yourself"))
			{
				app::GameData_PlayerOutfit* outfit = GetPlayerOutfit(GetPlayerData(*Game::pLocalPlayer));
				State.rpcQueue.push(new RpcRevive(*Game::pLocalPlayer));
				State.rpcQueue.push(new RpcForceColor(*Game::pLocalPlayer, outfit->fields.ColorId));
				framesPassed = 100;
				switch (State.mapType) {
				case Settings::MapType::Ship:
					State.rpcQueue.push(new RpcSnapTo({ 9.3840f, -6.0744f }));
					break;
				case Settings::MapType::Hq:
					State.rpcQueue.push(new RpcSnapTo({ 23.7700f, -1.5764f }));
					break;
				case Settings::MapType::Pb:
					State.rpcQueue.push(new RpcSnapTo({ 6.9000f, -14.0464f }));
					break;
				case Settings::MapType::Airship:
					State.rpcQueue.push(new RpcSnapTo({ -22.0990f, -1.1484f }));
					break;
				case Settings::MapType::Fungle:
					State.rpcQueue.push(new RpcSnapTo({ -15.3590f, -9.4194f }));
					break;
				}
				isReviving = true;
			}

			if (isReviving && framesPassed == 50)
			{
				State.rpcQueue.push(new RpcVent(*Game::pLocalPlayer, 1, false));
				framesPassed--;
			}
			else if (isReviving && framesPassed <= 0 && (*Game::pLocalPlayer)->fields.inVent) {
				State.rpcQueue.push(new RpcVent(*Game::pLocalPlayer, 1, true)); //for showing you as alive to ALL players
				framesPassed = -1;
				isReviving = false;
			}
			else if (isReviving) framesPassed--;

			ImGui::EndChild();
		}
	}
}