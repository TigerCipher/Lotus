using System;
using System.CodeDom;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Input;

namespace LotusEditor.Utility.Controls
{
    [TemplatePart(Name = "PART_textBlock", Type = typeof(TextBlock))]
    [TemplatePart(Name = "PART_textBox", Type = typeof(TextBox))]
    internal class NumberBox : Control
    {
        private double _originalValue;
        private bool _capturedMouse = false;
        private bool _valueChanged = false;
        private double _mouseXStart;
        private double _speedMultiplier;


        public static readonly DependencyProperty ValueProperty = DependencyProperty.Register(
            nameof(Value), typeof(string), typeof(NumberBox), new FrameworkPropertyMetadata(null, FrameworkPropertyMetadataOptions.BindsTwoWayByDefault));

        public string Value
        {
            get => (string)GetValue(ValueProperty);
            set => SetValue(ValueProperty, value);
        }

        public static readonly DependencyProperty MultiplierProperty = DependencyProperty.Register(
            nameof(Multiplier), typeof(double), typeof(NumberBox), new PropertyMetadata(1.0));

        public double Multiplier
        {
            get => (double)GetValue(MultiplierProperty);
            set => SetValue(MultiplierProperty, value);
        }

        static NumberBox()
        {
            DefaultStyleKeyProperty.OverrideMetadata(typeof(NumberBox), new FrameworkPropertyMetadata(typeof(NumberBox)));
        }

        public override void OnApplyTemplate()
        {
            base.OnApplyTemplate();

            if (GetTemplateChild("PART_textBlock") is not TextBlock textBlock) return;

            textBlock.MouseLeftButtonDown += TextBlock_OnMouseLeftDown;
            textBlock.MouseLeftButtonUp += TextBlock_OnMouseLeftUp;
            textBlock.MouseMove += TextBlock_OnMouseMove;
        }

        private void TextBlock_OnMouseMove(object sender, MouseEventArgs e)
        {
            if (!_capturedMouse) return;

            var mouseX = e.GetPosition(this).X;
            var delta = mouseX - _mouseXStart;
            if (Math.Abs(delta) > SystemParameters.MinimumHorizontalDragDistance)
            {
                if (Keyboard.Modifiers.HasFlag(ModifierKeys.Control)) _speedMultiplier = 0.001;
                else if (Keyboard.Modifiers.HasFlag(ModifierKeys.Shift)) _speedMultiplier = 1;
                else _speedMultiplier = 0.1;
                var newValue = _originalValue + (delta * _speedMultiplier * Multiplier);
                Value = newValue.ToString("0.#####");
                _valueChanged = true;
            }
        }

        private void TextBlock_OnMouseLeftUp(object sender, MouseButtonEventArgs e)
        {
            if (_capturedMouse)
            {
                Mouse.Capture(null);
                _capturedMouse = false;
                e.Handled = true;
                if (!_valueChanged && GetTemplateChild("PART_textBox") is TextBox textBox)
                {
                    textBox.Visibility = Visibility.Visible;
                    textBox.Focus();
                    textBox.SelectAll();
                }
            }
        }

        private void TextBlock_OnMouseLeftDown(object sender, MouseButtonEventArgs e)
        {
            double.TryParse(Value, out _originalValue);
            Mouse.Capture(sender as UIElement);
            _capturedMouse = true;
            _valueChanged = false;
            e.Handled = true;
            _mouseXStart = e.GetPosition(this).X;
        }
    }
}
