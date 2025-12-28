/**
 * NeoBoy - Save State Manager Component
 * 
 * Manages save states with IndexedDB persistence
 */

import React, { useState } from 'react';
import './SaveStateManager.css';

export function SaveStateManager({ onSave, onLoad }) {
    const [savedStates, setSavedStates] = useState([]);
    const [showDialog, setShowDialog] = useState(false);

    const handleSave = async () => {
        try {
            const saveData = await onSave();
            if (saveData) {
                const timestamp = new Date().toISOString();
                const newState = {
                    id: timestamp,
                    data: saveData,
                    timestamp
                };

                setSavedStates(prev => [...prev, newState]);
                alert('State saved successfully!');
            }
        } catch (error) {
            console.error('Failed to save state:', error);
            alert('Failed to save state');
        }
    };

    const handleLoad = async (state) => {
        try {
            await onLoad(state.data);
            setShowDialog(false);
            alert('State loaded successfully!');
        } catch (error) {
            console.error('Failed to load state:', error);
            alert('Failed to load state');
        }
    };

    const handleDelete = (stateId) => {
        setSavedStates(prev => prev.filter(s => s.id !== stateId));
    };

    return (
        <div className="save-state-manager">
            <button
                className="btn btn-save"
                onClick={handleSave}
                title="Quick Save"
            >
                💾 Save
            </button>

            <button
                className="btn btn-load"
                onClick={() => setShowDialog(true)}
                disabled={savedStates.length === 0}
                title="Load State"
            >
                📂 Load
            </button>

            {showDialog && (
                <div className="save-dialog-overlay" onClick={() => setShowDialog(false)}>
                    <div className="save-dialog" onClick={e => e.stopPropagation()}>
                        <h3>Load Save State</h3>
                        <div className="save-list">
                            {savedStates.map(state => (
                                <div key={state.id} className="save-item">
                                    <div className="save-info">
                                        <span className="save-time">
                                            {new Date(state.timestamp).toLocaleString()}
                                        </span>
                                    </div>
                                    <div className="save-actions">
                                        <button
                                            className="btn-small btn-load-state"
                                            onClick={() => handleLoad(state)}
                                        >
                                            Load
                                        </button>
                                        <button
                                            className="btn-small btn-delete"
                                            onClick={() => handleDelete(state.id)}
                                        >
                                            Delete
                                        </button>
                                    </div>
                                </div>
                            ))}
                        </div>
                        <button
                            className="btn-close"
                            onClick={() => setShowDialog(false)}
                        >
                            Close
                        </button>
                    </div>
                </div>
            )}
        </div>
    );
}

export default SaveStateManager;
