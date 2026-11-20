#!/usr/bin/env python3
"""Production CLI replay checks; never edits gameplay state to arrange outcomes."""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

PARSER = argparse.ArgumentParser()
PARSER.add_argument("--tool", type=Path, required=True)
PARSER.add_argument("--world", type=Path, required=True)
ARGS = PARSER.parse_args()
TOOL = ARGS.tool.resolve()
WORLD = ARGS.world.resolve()

class StarterJourneyReplayCliTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="embervale journey ")
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.scratch = self.root / "private saves"
        self.scratch.mkdir()
        (self.root / "saves").mkdir()
        self.sentinel = self.root / "saves/development-player.save"
        self.sentinel.write_bytes(b"existing player profile must stay untouched\n")
        self.world = self.root / "world with spaces.json"
        self.world.write_bytes(WORLD.read_bytes())
        self.environment = os.environ.copy()
        for key in ("TMPDIR", "TEMP", "TMP"):
            self.environment[key] = str(self.scratch)

    def run_tool(self, *arguments):
        return subprocess.run([str(TOOL), *map(str, arguments)], cwd=self.root,
            env=self.environment, capture_output=True, text=True, timeout=60)

    def assert_isolated(self):
        self.assertEqual(self.sentinel.read_bytes(), b"existing player profile must stay untouched\n")
        self.assertEqual(sorted(p.relative_to(self.root).as_posix() for p in self.root.rglob("*") if p.is_file()),
            ["saves/development-player.save", "world with spaces.json"])
        self.assertEqual(list(self.scratch.iterdir()), [])

    def test_full_journey_repeatable_and_isolated(self):
        first = self.run_tool("--replay-starter-journey", "--world", self.world)
        second = self.run_tool("--replay-starter-journey", "--world", self.world)
        self.assertEqual(first.returncode, 0, first.stderr)
        self.assertEqual(second.returncode, 0, second.stderr)
        self.assertEqual(first.stdout, second.stdout, "Fresh process replay must be reproducible")
        result = json.loads(first.stdout)
        self.assertEqual(result["kind"], "authoritative-starter-journey-replay")
        preparation, journey = result["preparation"], result["journey"]
        self.assertEqual(preparation["profile"], "fresh")
        self.assertEqual(preparation["starting_coins"], 0)
        self.assertFalse(preparation["injected_materials_or_xp"])
        self.assertLess(preparation["first_shield_equipped_tick"], preparation["first_sword_equipped_tick"])
        self.assertLess(preparation["first_sword_equipped_tick"], preparation["simulation_ticks"])
        self.assertTrue(journey["all_checks_passed"])
        self.assertTrue(journey["authoritative_commands_only"])
        self.assertFalse(journey["injected_damage_stats_items_or_xp"])
        self.assertFalse(journey["human_pacing_measured"])
        self.assertFalse(journey["visible_playthrough_performed"])
        self.assertGreater(journey["first_combat_damage_dealt"], 0)
        self.assertGreater(journey["first_combat_damage_taken"], 0)
        self.assertGreater(journey["first_combat_minimum_health"], 0)
        self.assertGreater(journey["food_consumed"], 0)
        self.assertGreater(journey["actual_health_restored"], 0)
        for key in ("full_health_food_rejected_without_consumption", "waited_for_post_combat_bank",
                    "waited_for_return_to_mara", "dead_save_rejected_preserving_checkpoint",
                    "protected_items_conserved", "recovery_reclaimed_exactly_once",
                    "ground_pickup_exercised_with_owned_loot_type"):
            self.assertTrue(journey[key], key)
        self.assertEqual(journey["monster_loot_delivery"], "direct_to_inventory")
        self.assertGreaterEqual(journey["loot_items_received_and_banked"], 1)
        self.assertGreater(journey["respawn_delay_ticks"], 0)
        self.assertLess(journey["total_simulation_ticks"], 20000)
        self.assertEqual(journey["primary_play_sessions"], 2)
        self.assertTrue(journey["world_and_rng_restart_on_reload"])
        checks = {c["stage"]: c for c in journey["save_round_trips"]}
        self.assertEqual(set(checks), {"prepared", "awaiting-bank", "awaiting-mara",
                                     "post-combat", "pending-recovery", "recovered"})
        for check in checks.values():
            self.assertTrue(check["canonical_bytes_equal"])
            self.assertTrue(check["live_state_unchanged"])
            self.assertTrue(check["items_equal"])
        self.assertTrue(checks["awaiting-bank"]["reload_idle_tick_checked"])
        self.assertTrue(checks["awaiting-mara"]["reload_idle_tick_checked"])
        self.assertTrue(checks["pending-recovery"]["pending_recovery_pile"])
        self.assertGreater(checks["pending-recovery"]["pile_remaining_ticks"], 0)
        self.assertFalse(checks["recovered"]["pending_recovery_pile"])
        stages = journey["stages"]
        ticks = [s["cumulative_ticks"] for s in stages]
        self.assertEqual(ticks, sorted(ticks))
        self.assertGreater(journey["total_simulation_ticks"], journey["first_kill_tick"])
        self.assert_isolated()

    def test_upgrade_mode_keeps_existing_scope_and_creates_no_saves(self):
        run = self.run_tool("--replay-starter-upgrade", "--world", self.world)
        self.assertEqual(run.returncode, 0, run.stderr)
        report = json.loads(run.stdout)
        self.assertEqual(report["kind"], "authoritative-starter-upgrade-replay")
        self.assertEqual(report["tutorial_step"], "SAFE_COMBAT")
        self.assertNotIn("journey", report)
        self.assert_isolated()

    def test_rejects_attempt_to_select_an_existing_player_save(self):
        run = self.run_tool("--replay-starter-journey", "--save", self.sentinel)
        self.assertEqual(run.returncode, 2, run.stderr)
        self.assertEqual(run.stdout, "")
        self.assert_isolated()

    def test_requires_a_selected_world_for_the_full_journey(self):
        run = self.run_tool("--replay-starter-journey")
        self.assertEqual(run.returncode, 2, run.stderr)
        self.assertEqual(run.stdout, "")
        self.assert_isolated()

    def test_missing_world_fails_without_touching_profiles(self):
        run = self.run_tool("--replay-starter-journey", "--world", self.root / "missing.json")
        self.assertEqual(run.returncode, 1, run.stderr)
        self.assertEqual(run.stdout, "")
        self.assert_isolated()

if __name__ == "__main__":
    unittest.main(argv=[sys.argv[0]])
