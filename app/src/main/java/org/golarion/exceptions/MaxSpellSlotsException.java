package org.golarion.exceptions;

public class MaxSpellSlotsException extends RuntimeException
{
    public MaxSpellSlotsException(String errorMessage)
    {
        super(errorMessage);
    }

    public MaxSpellSlotsException(Throwable e)
    {
        super(e);
    }

    public MaxSpellSlotsException(String errorMessage, Throwable e)
    {
        super(errorMessage, e);
    }
}
