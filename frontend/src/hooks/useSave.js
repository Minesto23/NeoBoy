/**
 * NeoBoy - Save Hook
 * 
 * Handles save state persistence using IndexedDB
 * Saves and loads emulator state
 */

import { useCallback } from 'react';

const DB_NAME = 'NeoBoyDB';
const STORE_NAME = 'saves';

/**
 * Initialize IndexedDB
 */
const initDB = () => {
    return new Promise((resolve, reject) => {
        const request = indexedDB.open(DB_NAME, 1);

        request.onerror = () => reject(request.error);
        request.onsuccess = () => resolve(request.result);

        request.onupgradeneeded = (event) => {
            const db = event.target.result;
            if (!db.objectStoreNames.contains(STORE_NAME)) {
                db.createObjectStore(STORE_NAME, { keyPath: 'id' });
            }
        };
    });
};

/**
 * Save data to IndexedDB
 */
const saveToIndexedDB = async (key, data) => {
    const db = await initDB();
    const tx = db.transaction(STORE_NAME, 'readwrite');
    const store = tx.objectStore(STORE_NAME);

    await store.put({
        id: key,
        data: data,
        timestamp: Date.now()
    });

    await tx.complete;
};

/**
 * Load data from IndexedDB
 */
const loadFromIndexedDB = async (key) => {
    const db = await initDB();
    const tx = db.transaction(STORE_NAME, 'readonly');
    const store = tx.objectStore(STORE_NAME);

    const result = await store.get(key);
    return result?.data;
};

export function useSave(wasmCore, coreType) {
    /**
     * Save current emulator state
     */
    const saveState = useCallback(async () => {
        if (!wasmCore) {
            console.error('WASM core not initialized');
            return null;
        }

        try {
            const stateData = wasmCore.save();
            const key = `${coreType}_state`;
            await saveToIndexedDB(key, stateData);

            console.log(`State saved for ${coreType}`);
            return stateData;
        } catch (error) {
            console.error('Failed to save state:', error);
            throw error;
        }
    }, [wasmCore, coreType]);

    /**
     * Load saved emulator state
     */
    const loadSavedState = useCallback(async () => {
        if (!wasmCore) {
            console.error('WASM core not initialized');
            return false;
        }

        try {
            const key = `${coreType}_state`;
            const stateData = await loadFromIndexedDB(key);

            if (!stateData) {
                console.warn('No saved state found');
                return false;
            }

            const success = wasmCore.loadSave(stateData);
            console.log(`State loaded for ${coreType}: ${success}`);
            return success;
        } catch (error) {
            console.error('Failed to load state:', error);
            return false;
        }
    }, [wasmCore, coreType]);

    return { saveState, loadSavedState };
}
