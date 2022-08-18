﻿using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace LotusEditor.Utility
{
    interface IUndoRedo
    {
        string Name { get; }

        void Undo();
        void Redo();
    }

    internal class UndoRedoAction : IUndoRedo
    {
        private Action _undoAction;
        private Action _redoAction;

        public string Name { get; }

        public UndoRedoAction(string name)
        {
            Name = name;
        }

        public UndoRedoAction(string name, Action undo, Action redo) : this(name)
        {
            Debug.Assert(undo != null && redo != null);
            _undoAction = undo;
            _redoAction = redo;
        }

        public void Undo() => _undoAction();

        public void Redo() => _redoAction();
    }


    /// <summary>
    /// Provides a typical undo/redo system
    /// </summary>
    internal class History
    {
        private readonly ObservableCollection<IUndoRedo> _undoList = new();
        private readonly ObservableCollection<IUndoRedo> _redoList = new();

        public ReadOnlyObservableCollection<IUndoRedo> UndoList { get; }
        public ReadOnlyObservableCollection<IUndoRedo> RedoList { get; }


        public History()
        {
            UndoList = new ReadOnlyObservableCollection<IUndoRedo>(_undoList);
            RedoList = new ReadOnlyObservableCollection<IUndoRedo>(_redoList);
        }
        
        public void Reset()
        {
            _undoList.Clear();
            _redoList.Clear();
        }

        public void Undo()
        {
            if (!_undoList.Any()) return;
            var cmd = _undoList.Last();
            _undoList.RemoveAt(_undoList.Count - 1);
            cmd.Undo();
            _redoList.Insert(0, cmd);
        }

        public void Redo()
        {
            if (!_redoList.Any()) return;
            var cmd = _redoList.First();
            _redoList.RemoveAt(0);
            cmd.Redo();
            _undoList.Add(cmd);
        }

        public void AddUndoRedoAction(IUndoRedo cmd)
        {
            _undoList.Add(cmd);
            _redoList.Clear();
        }
    }
}
