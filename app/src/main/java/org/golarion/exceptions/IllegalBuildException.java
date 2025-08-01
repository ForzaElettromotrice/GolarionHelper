package org.golarion.exceptions;

public class IllegalBuildException extends RuntimeException
{
    public IllegalBuildException(String errorMessage)
    {
        super(errorMessage);
    }

    public IllegalBuildException(Throwable e)
    {
        super(e);
    }

    public IllegalBuildException(String errorMessage, Throwable e)
    {
        super(errorMessage, e);
    }
}
