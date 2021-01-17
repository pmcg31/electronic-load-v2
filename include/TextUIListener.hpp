#ifndef __H_TEXTUILISTENER__
#define __H_TEXTUILISTENER__

class TextUI;

class TextUIListener
{
public:
    virtual void desiredCurrentChanged(TextUI *source, double desiredCurrent) = 0;
    virtual void enabledChanged(TextUI *source, bool isEnabled) = 0;
};

#endif