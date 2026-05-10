package org.golarion.model.character.modifier;

import lombok.Getter;
import lombok.NonNull;

import java.util.regex.Pattern;

public class Expression
{
    private final class Parser
    {
        private int position;

        private int parseExpression()
        {
            int value = parseTerm();

            while (true)
            {
                skipWhitespace();
                if (match('+'))
                {
                    value += parseTerm();
                    continue;
                }

                if (match('-'))
                {
                    value -= parseTerm();
                    continue;
                }

                return value;
            }
        }

        private int parseTerm()
        {
            int value = parseFactor();

            while (true)
            {
                skipWhitespace();
                if (match('*'))
                {
                    value *= parseFactor();
                    continue;
                }

                if (match('/'))
                {
                    int divisor = parseFactor();
                    if (divisor == 0)
                    {
                        throw new IllegalArgumentException("Division by zero is not allowed");
                    }

                    value /= divisor;
                    continue;
                }

                return value;
            }
        }

        private int parseFactor()
        {
            skipWhitespace();

            if (match('+'))
            {
                return parseFactor();
            }

            if (match('-'))
            {
                return -parseFactor();
            }

            if (match('('))
            {
                int value = parseExpression();
                skipWhitespace();
                if (!match(')'))
                {
                    throw new IllegalArgumentException("Missing closing ')' at position " + position);
                }

                return value;
            }

            if (match('@'))
            {
                return parseVariable();
            }

            if (isDigit(peek()))
            {
                return parseNumber();
            }

            throw new IllegalArgumentException("Unexpected token at position " + position);
        }

        private int parseNumber()
        {
            int start = position;
            while (isDigit(peek()))
            {
                position++;
            }

            return Integer.parseInt(expression.substring(start, position));
        }

        private int parseVariable()
        {
            int start = position;
            while (isVariableCharacter(peek()))
            {
                position++;
            }

            if (start == position)
            {
                throw new IllegalArgumentException("Missing variable name after '@' at position " + start);
            }

            String variableName = expression.substring(start, position);
            return targetManager.resolveValue(variableName);
        }

        private void skipWhitespace()
        {
            while (Character.isWhitespace(peek()))
            {
                position++;
            }
        }

        private boolean match(char expected)
        {
            if (peek() != expected)
            {
                return false;
            }

            position++;
            return true;
        }

        private char peek()
        {
            if (isAtEnd())
            {
                return '\0';
            }

            return expression.charAt(position);
        }

        private boolean isAtEnd()
        {
            return position >= expression.length();
        }

        private boolean isDigit(char value)
        {
            return value >= '0' && value <= '9';
        }

        private boolean isVariableCharacter(char value)
        {
            return Character.isLetterOrDigit(value) || value == '_';
        }
    }

    @Getter
    private final String expression;
    private final TargetManager targetManager;

    public Expression(@NonNull TargetManager targetManager, @NonNull String expression)
    {
        this.targetManager = targetManager;
        this.expression = expression.trim();
    }

    public int getValue()
    {
        Parser parser = new Parser();
        int value = parser.parseExpression();
        if (!parser.isAtEnd())
        {
            throw new IllegalArgumentException("Unexpected token at position " + parser.position);
        }

        return value;
    }

    public boolean referencesVariable(@NonNull String variableName)
    {
        Pattern pattern = Pattern.compile("(?i)@" + Pattern.quote(variableName) + "(?![A-Za-z0-9_])");
        return pattern.matcher(expression).find();
    }
}
