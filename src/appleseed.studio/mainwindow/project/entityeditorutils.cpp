
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2014-2015 Francois Beaune, The appleseedhq Organization
// Copyright (c) 2014-2015 Marius Avram, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Interface header.
#include "entityeditorutils.h"

// appleseed.studio headers.
#include "utility/doubleslider.h"
#include "utility/interop.h"

// Qt headers.
#include <QLineEdit>

// Standard headers.
#include <cmath>

using namespace foundation;
using namespace std;

namespace appleseed {
namespace studio {

//
// LineEditDoubleSliderAdaptor class implementation.
//

LineEditDoubleSliderAdaptor::LineEditDoubleSliderAdaptor(
    QLineEdit*      line_edit,
    DoubleSlider*   slider)
  : QObject(line_edit)
  , m_line_edit(line_edit)
  , m_slider(slider)
{
    slot_set_slider_value(m_line_edit->text());
}

void LineEditDoubleSliderAdaptor::slot_set_line_edit_value(const double value)
{
    // Don't block signals here, for live edit to work we want the line edit to signal changes.
    m_line_edit->setText(QString("%1").arg(value));
}

void LineEditDoubleSliderAdaptor::slot_set_slider_value(const QString& value)
{
    m_slider->blockSignals(true);

    const double new_value = value.toDouble();

    // Adjust range if the new value is outside the current range.
    if (new_value < m_slider->minimum() ||
        new_value > m_slider->maximum())
        adjust_slider(new_value);

    m_slider->setValue(new_value);
    m_slider->blockSignals(false);
}

void LineEditDoubleSliderAdaptor::slot_apply_slider_value()
{
    m_slider->blockSignals(true);

    const double new_value = m_line_edit->text().toDouble();

    // Adjust range if the new value is outside the current range,
    // or if a value of a significantly smaller magnitude was entered.
    if (new_value < m_slider->minimum() ||
        new_value > m_slider->maximum() ||
        abs(new_value) < (m_slider->maximum() - m_slider->minimum()) / 3.0)
        adjust_slider(new_value);

    m_slider->setValue(new_value);
    m_slider->blockSignals(false);
}

void LineEditDoubleSliderAdaptor::adjust_slider(const double new_value)
{
    const double new_min = new_value >= 0.0 ? 0.0 : -2.0 * abs(new_value);
    const double new_max = new_value == 0.0 ? 1.0 : +2.0 * abs(new_value);
    m_slider->setRange(new_min, new_max);
    m_slider->setPageStep((new_max - new_min) / 10.0);
}


//
// ForwardColorChangedSignal class implementation.
//

ForwardColorChangedSignal::ForwardColorChangedSignal(
    QObject*        parent,
    const QString&  widget_name)
  : QObject(parent)
  , m_widget_name(widget_name)
{
}

void ForwardColorChangedSignal::slot_color_changed(const QColor& color)
{
    emit signal_color_changed(m_widget_name, color);
}

}   // namespace studio
}   // namespace appleseed
