"use client";

import { useState, useEffect, useCallback } from "react";
import { CreatureState } from "@/types";
import { updateCreatureState, createInitialCreatureState } from "@/lib/mood";
import { getCreatureState, saveCreatureState } from "@/lib/storage";

export function useCreatureMood() {
  const [creature, setCreature] = useState<CreatureState>(
    createInitialCreatureState()
  );

  // Load saved state on mount
  useEffect(() => {
    const saved = getCreatureState();
    // Recalculate mood based on time elapsed since last save
    const updated = updateCreatureState(saved);
    setCreature(updated);
    saveCreatureState(updated);
  }, []);

  // Periodically update mood (happiness decays over time)
  useEffect(() => {
    const interval = setInterval(() => {
      setCreature((prev) => {
        const updated = updateCreatureState(prev);
        saveCreatureState(updated);
        return updated;
      });
    }, 60_000); // Update every minute

    return () => clearInterval(interval);
  }, []);

  const recordMessage = useCallback((count: number = 1) => {
    setCreature((prev) => {
      const updated = updateCreatureState(prev, count);
      saveCreatureState(updated);
      return updated;
    });
  }, []);

  const resetCreature = useCallback(() => {
    const fresh = createInitialCreatureState();
    setCreature(fresh);
    saveCreatureState(fresh);
  }, []);

  return { creature, recordMessage, resetCreature };
}
